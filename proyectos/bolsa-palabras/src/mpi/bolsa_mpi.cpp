// Proyecto Clausura: Bolsa de palabras. VERSION MPI.
//
// Uso:  mpirun -np <q> ./bolsa_mpi <urls.txt> <salida.csv>
//
// En UNA sola corrida hace tres cosas:
//   1) El proceso 0 ejecuta la version SERIAL (descarga + conteo + CSV) y mide T_serial.
//      Su CSV se guarda en <salida.csv>.serial.csv.
//   2) Todos los procesos ejecutan la version PARALELA y se mide T_paralelo.
//      Su CSV se guarda en <salida.csv>.
//   3) El proceso 0 compara las dos matrices e imprime los tiempos y el speedup
//      (T_serial / T_paralelo).
//
// Version paralela (tres fases):
//   1) DESCARGA. Reparto estatico por libro: el proceso r descarga los libros i con i % q == r
//      (archivo libro_<i>.txt, con el indice GLOBAL i). Funciona para cualquier k y q.
//   2) CONTEO. Repartir por libro no balancea: los libros pesan muy distinto (uno solo puede ser
//      el 75% del texto) y el proceso que lo recibe tarda mas que todos los demas. Por eso cada
//      dueno corta sus libros en TROZOS de ~256 KB (en un byte separador, para no partir
//      palabras) y los reparte por turnos entre los q procesos con MPI_Alltoallv. Cada proceso
//      cuenta los trozos que recibe y suma los conteos por libro. Contar por trozos y sumar da
//      exactamente el mismo resultado que contar el libro completo.
//   3) UNION. Cada proceso aplana sus conteos en bytes y se juntan en el proceso 0 con
//      MPI_Gather (tamanos) + MPI_Gatherv (datos). El proceso 0 SUMA los conteos de cada libro,
//      arma el vocabulario y escribe el CSV (esta parte es serial: Amdahl).
//
// Funciones compartidas con la version serial: ../comun/bolsa_comun.h

#include <mpi.h>

#include <cstring>

#include "../comun/bolsa_comun.h"

// ---------------------------------------------------------------------------------------
// Empaquetado: MPI no sabe mandar map<string,int>, asi que se aplana a bytes.
// Formato de un libro:  [int indice][int n_palabras] y luego n_palabras veces
//                       [palabra terminada en '\0'][int conteo]
// ---------------------------------------------------------------------------------------
static void agregarInt(string& buf, int x) {
    buf.append(reinterpret_cast<const char*>(&x), sizeof(int));
}

static void empaquetarLibro(string& buf, int indice, const map<string, int>& conteo) {
    agregarInt(buf, indice);
    agregarInt(buf, (int)conteo.size());
    for (const auto& par : conteo) {
        buf.append(par.first);
        buf.push_back('\0');
        agregarInt(buf, par.second);
    }
}

// Lee todos los libros contenidos en 'datos' (de 'n' bytes) y SUMA sus conteos en conteos[indice].
static void desempaquetar(const char* datos, size_t n, vector<map<string, int>>& conteos) {
    size_t pos = 0;
    auto leerInt = [&]() {
        int x;
        memcpy(&x, datos + pos, sizeof(int));
        pos += sizeof(int);
        return x;
    };
    while (pos < n) {
        int indice = leerInt();
        int n_palabras = leerInt();
        map<string, int>& destino = conteos[indice];
        for (int w = 0; w < n_palabras; w++) {
            string palabra(datos + pos);  // hasta el '\0'
            pos += palabra.size() + 1;
            destino[palabra] += leerInt();  // suma: un libro puede venir de varios procesos
        }
    }
}

// Tamano nominal de un trozo de texto (bytes). Mas chico = mejor balance pero mas mensajes.
static const size_t TROZO = 256 * 1024;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, q;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &q);

    if (argc != 3) {
        if (rank == 0) cout << "Uso: mpirun -np <q> " << argv[0] << " <urls.txt> <salida.csv>\n";
        MPI_Finalize();
        return 1;
    }
    string ruta_urls = argv[1];
    string ruta_salida = argv[2];

    // Todos leen el listado (es chico); asi no hace falta mandar las URLs.
    vector<string> urls = leerUrls(ruta_urls);
    int k = (int)urls.size();

    // ---------------------------- 1) SERIAL (solo el proceso 0) ----------------------------
    double s_descarga = 0, s_proceso = 0;
    vector<map<string, int>> conteos_serial;
    if (rank == 0) {
        double t0 = MPI_Wtime();
        vector<string> textos(k);
        for (int i = 0; i < k; i++) textos[i] = obtenerTexto(urls[i], i);
        double t1 = MPI_Wtime();
        conteos_serial.resize(k);
        for (int i = 0; i < k; i++) conteos_serial[i] = contarPalabras(textos[i]);
        vector<string> vocab = construirVocabulario(conteos_serial);
        escribirCsv(ruta_salida + ".serial.csv", vocab, conteos_serial);
        double t2 = MPI_Wtime();
        s_descarga = t1 - t0;
        s_proceso = t2 - t1;
    }

    // ---------------------------- 2) PARALELO (todos) --------------------------------------
    MPI_Barrier(MPI_COMM_WORLD);  // todos empiezan juntos
    double p0 = MPI_Wtime();

    // 2a) descargar los libros propios (libro i si i % q == rank)
    vector<int> mis_libros;
    vector<string> mis_textos;
    for (int i = rank; i < k; i += q) {
        mis_libros.push_back(i);
        mis_textos.push_back(obtenerTexto(urls[i], i));
    }
    MPI_Barrier(MPI_COMM_WORLD);  // separa descarga de procesamiento para poder medirlas
    double p1 = MPI_Wtime();

    // 2b) repartir trozos de texto entre todos los procesos (MPI_Alltoallv)
    // Para cada destino se arma un buffer con trozos: [int libro][int largo][bytes del trozo].
    vector<string> para(q);
    int turno = rank;  // cada dueno empieza en un destino distinto
    for (size_t j = 0; j < mis_libros.size(); j++) {
        const string& t = mis_textos[j];
        size_t pos = 0;
        while (pos < t.size()) {
            size_t fin = min(pos + TROZO, t.size());
            // alargar el trozo hasta un byte que NO sea a-z/A-Z/0-9, para no partir una palabra
            while (fin < t.size() && (unsigned char)t[fin] < 128 && isalnum((unsigned char)t[fin])) fin++;
            string& buf = para[turno % q];
            agregarInt(buf, mis_libros[j]);
            agregarInt(buf, (int)(fin - pos));
            buf.append(t, pos, fin - pos);
            turno++;
            pos = fin;
        }
    }
    mis_textos.clear();

    vector<int> env(q), rec(q), env_d(q, 0), rec_d(q, 0);
    for (int r = 0; r < q; r++) env[r] = (int)para[r].size();
    MPI_Alltoall(env.data(), 1, MPI_INT, rec.data(), 1, MPI_INT, MPI_COMM_WORLD);
    string envio, recibo_trozos;
    int tot_env = 0, tot_rec = 0;
    for (int r = 0; r < q; r++) {
        env_d[r] = tot_env;
        rec_d[r] = tot_rec;
        tot_env += env[r];
        tot_rec += rec[r];
        envio += para[r];
    }
    para.clear();
    recibo_trozos.resize(tot_rec);
    MPI_Alltoallv(envio.data(), env.data(), env_d.data(), MPI_CHAR,
                  tot_rec > 0 ? &recibo_trozos[0] : nullptr, rec.data(), rec_d.data(), MPI_CHAR,
                  MPI_COMM_WORLD);
    envio.clear();

    // 2c) contar los trozos recibidos, sumando por libro
    map<int, map<string, int>> mios;
    for (size_t pos = 0; pos < recibo_trozos.size();) {
        int libro, largo;
        memcpy(&libro, recibo_trozos.data() + pos, sizeof(int));
        memcpy(&largo, recibo_trozos.data() + pos + sizeof(int), sizeof(int));
        pos += 2 * sizeof(int);
        contarPalabrasEn(mios[libro], recibo_trozos.data() + pos, (size_t)largo);
        pos += largo;
    }
    recibo_trozos.clear();
    string buffer;
    for (const auto& par : mios) empaquetarLibro(buffer, par.first, par.second);
    MPI_Barrier(MPI_COMM_WORLD);
    double p15 = MPI_Wtime();  // fin del conteo; empieza la union en el proceso 0

    // 2d) juntar en el proceso 0
    int mi_tam = (int)buffer.size();
    vector<int> tams(q), desplaz(q, 0);
    MPI_Gather(&mi_tam, 1, MPI_INT, tams.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    string recibido;
    if (rank == 0) {
        int total = 0;
        for (int r = 0; r < q; r++) {
            desplaz[r] = total;
            total += tams[r];
        }
        recibido.resize(total);
    }
    MPI_Gatherv(buffer.data(), mi_tam, MPI_CHAR, rank == 0 ? &recibido[0] : nullptr, tams.data(),
                desplaz.data(), MPI_CHAR, 0, MPI_COMM_WORLD);

    // 2e) el proceso 0 suma los conteos, arma la matriz y escribe el CSV
    vector<map<string, int>> conteos_par;
    if (rank == 0) {
        conteos_par.resize(k);
        desempaquetar(recibido.data(), recibido.size(), conteos_par);
        vector<string> vocab = construirVocabulario(conteos_par);
        escribirCsv(ruta_salida, vocab, conteos_par);
    }
    MPI_Barrier(MPI_COMM_WORLD);  // el tiempo termina cuando el CSV ya esta escrito
    double p2 = MPI_Wtime();

    // ---------------------------- 3) RESULTADOS (proceso 0) --------------------------------
    if (rank == 0) {
        double p_descarga = p1 - p0, p_proceso = p2 - p1, p_union = p2 - p15;
        double t_serial = s_descarga + s_proceso;
        double t_par = p_descarga + p_proceso;
        bool iguales = (conteos_serial == conteos_par);

        cout << "libros=" << k << " procesos=" << q << "\n";
        cout << "matrices serial y paralela identicas: " << (iguales ? "si" : "NO") << "\n";
        cout << "                      serial    paralelo   speedup\n";
        cout << "descarga (s)       " << s_descarga << "   " << p_descarga << "   "
             << s_descarga / p_descarga << "\n";
        cout << "procesamiento (s)  " << s_proceso << "   " << p_proceso << "   "
             << s_proceso / p_proceso << "\n";
        cout << "  de ello, union serial en el proceso 0 (sumar + vocabulario + CSV): " << p_union
             << " s\n";
        cout << "TOTAL (s)          " << t_serial << "   " << t_par << "   " << t_serial / t_par
             << "\n";
        cout << "SPEEDUP (total, con descarga) = " << t_serial / t_par << "\n";
        if (!iguales) cerr << "ERROR: las matrices no coinciden\n";
    }

    MPI_Finalize();
    return 0;
}
