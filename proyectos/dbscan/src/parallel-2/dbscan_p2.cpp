// dbscan_p2.cpp
//
// Version paralela P2 del DBSCAN "ingenuo" de 2 pasos: se DIVIDE la matriz
// (el plano donde viven los puntos) en `k` particiones y cada particion se
// resuelve en paralelo.
//
//   Paso 1: clasificar cada punto como CORE (de primer orden) si tiene al
//           menos `min_samples` puntos (incluyendose a si mismo) a una
//           distancia euclidiana <= eps. Si no, se clasifica como RUIDO.
//
//   Paso 2: para cada punto que quedo como RUIDO, revisar si esta a
//           distancia <= eps de algun punto CORE de primer orden. Si es asi,
//           se reclasifica como CORE (de segundo orden).
//
// Como se divide (igual que el dibujo de la diapositiva 11):
//   El rectangulo que contiene a todos los puntos se parte en una cuadricula
//   de filas x columnas con filas * columnas = k:
//     k = 2 -> 1x2,  k = 4 -> 2x2,  k = 8 -> 2x4,
//     k = 16 -> 4x4, k = 32 -> 4x8, k = 64 -> 8x8.
//   Cada punto PERTENECE a exactamente una celda (su particion).
//
// Como se mantiene la consistencia (la pregunta del profesor):
//   Un punto pegado a la orilla de su celda puede tener vecinos del OTRO
//   lado de la frontera. Si cada particion solo mirara sus propios puntos,
//   ese punto perderia vecinos y se marcaria como ruido por error.
//   Solucion: "halo" (zona fantasma). Cada particion tiene dos listas:
//     - propios:    los puntos que le pertenecen. Solo a ESTOS les escribe
//                   etiqueta.
//     - candidatos: sus propios puntos MAS los puntos de otras celdas que
//                   estan a menos de eps de su rectangulo (el halo). Solo
//                   se LEEN, para contar vecinos o buscar un core cercano.
//   Cualquier vecino real de un punto propio (distancia <= eps) esta dentro
//   del rectangulo agrandado eps por cada lado, asi que esta en candidatos:
//   cada particion ve EXACTAMENTE los mismos vecinos que veria la serial.
//   Y como cada punto se etiqueta en una sola particion, "unir" los
//   resultados no requiere nada: todas escriben en el mismo arreglo global
//   es_core, cada una en posiciones distintas, sin conflictos.
//
// Entrada:  un CSV sin encabezado, con columnas x,y.
// Salida:   un CSV sin encabezado, con columnas x,y,label
//           (label = 0 ruido, 1 core).
//
// Uso (Mac con libomp de Homebrew):
//   clang++ -std=c++17 -O2 -Xpreprocessor -fopenmp -I/usr/local/opt/libomp/include \
//       -L/usr/local/opt/libomp/lib -lomp dbscan_p2.cpp -o dbscan_p2.out
//   ./dbscan_p2.out 4000_data.csv 4000_results.csv 0.03 10 8 4

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <omp.h>
#include <cstdlib>
#include <cctype>
#include <vector>

using namespace std;

// Regresa true si la linea empieza con un numero (digito, signo o punto
// decimal). leerPuntos() la usa para saltarse las lineas que no son datos:
// lineas vacias y un encabezado como "x,y" si el CSV trae uno. (El CSV de
// DBSCAN_noise.ipynb no trae encabezado, pero asi no truena stod si alguien
// guarda los puntos con pandas, que si lo pone.)
bool empiezaConNumero(const string& linea) {
    size_t p = linea.find_first_not_of(" \t");
    if (p == string::npos) return false;  // linea vacia o solo espacios
    unsigned char c = linea[p];  // unsigned: isdigit no acepta char negativo
    return isdigit(c) || c == '-' || c == '+' || c == '.';
}

// Lee un CSV de dos columnas (x,y) y reserva arreglos dinamicos para
// almacenar las coordenadas. Devuelve el numero de puntos leidos por
// referencia en n_points. (Misma funcion que en la version serial.)
void leerPuntos(const string& ruta, double*& x, double*& y, int& n_points) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo de entrada: " << ruta << "\n";
        exit(1);
    }

    // Primera pasada: contar cuantas lineas (puntos) hay.
    string linea;
    n_points = 0;
    while (getline(archivo, linea)) {
        if (empiezaConNumero(linea)) n_points++;
    }

    x = new double[n_points];
    y = new double[n_points];

    // Segunda pasada: releer el archivo desde el inicio y parsear "x,y".
    archivo.clear();
    archivo.seekg(0, ios::beg);

    int i = 0;
    while (getline(archivo, linea)) {
        if (!empiezaConNumero(linea)) continue;
        stringstream ss(linea);
        string valor;

        getline(ss, valor, ',');
        x[i] = stod(valor);

        getline(ss, valor, ',');
        y[i] = stod(valor);

        i++;
    }

    archivo.close();
}

// Distancia euclidiana entre el punto i y el punto j.
inline double distancia(const double* x, const double* y, int i, int j) {
    double dx = x[i] - x[j];
    double dy = y[i] - y[j];
    return sqrt(dx * dx + dy * dy);
}

// Convierte k en una cuadricula filas x columnas lo mas "cuadrada" posible:
// filas = la mayor potencia de 2 con filas * filas <= k, columnas = k / filas.
// (k = 2 -> 1x2, 4 -> 2x2, 8 -> 2x4, 16 -> 4x4, 32 -> 4x8, 64 -> 8x8.)
void cuadricula(int k, int& filas, int& columnas) {
    filas = 1;
    while ((filas * 2) * (filas * 2) <= k) filas *= 2;
    columnas = k / filas;
}

int main(int argc, char** argv) {

    // Respecto a la serial se agregan 2 argumentos: el numero de hilos y k,
    // el numero de particiones ({2, 4, 8, 16, 32, 64} en el experimento).
    if (argc != 7) {
        cout << "Uso: " << argv[0]
             << " <entrada.csv> <salida.csv> <eps> <min_samples> <num_hilos> <k>\n";
        return 1;
    }

    string ruta_entrada  = argv[1];
    string ruta_salida   = argv[2];
    double eps           = atof(argv[3]);
    int min_samples      = atoi(argv[4]);
    int num_hilos        = atoi(argv[5]);
    int k                = atoi(argv[6]);

    int filas, columnas;
    cuadricula(k, filas, columnas);
    if (filas * columnas != k) {
        cerr << "k debe ser potencia de 2 (2, 4, 8, ..., 64)\n";
        return 1;
    }

    omp_set_num_threads(num_hilos);

    // --- Cargar los puntos ---
    double* x = nullptr;
    double* y = nullptr;
    int n_points = 0;
    leerPuntos(ruta_entrada, x, y, n_points);

    // es_core[i] == true  -> el punto i es core (de 1er o 2do orden)
    // es_core[i] == false -> el punto i es ruido/outlier
    bool* es_core = new bool[n_points];
    for (int i = 0; i < n_points; i++) es_core[i] = false;

    // Se mide desde aqui para que el tiempo incluya TODO lo que P2 hace de
    // mas respecto a la serial (armar las particiones y los halos), no solo
    // los dos pasos. Si no, la comparacion favoreceria a P2 injustamente.
    double inicio = omp_get_wtime();

    // --- Dividir el plano en k celdas ---
    // Rectangulo que contiene a todos los puntos.
    double x_min = x[0], x_max = x[0], y_min = y[0], y_max = y[0];
    for (int i = 1; i < n_points; i++) {
        x_min = min(x_min, x[i]);  x_max = max(x_max, x[i]);
        y_min = min(y_min, y[i]);  y_max = max(y_max, y[i]);
    }
    double ancho_celda = (x_max - x_min) / columnas;
    double alto_celda  = (y_max - y_min) / filas;

    // propios[c]: indices de los puntos que pertenecen a la celda c.
    // A cada punto le toca la celda (fila, columna) en la que cae; el min()
    // manda a los puntos que estan justo en el borde derecho/superior a la
    // ultima celda en vez de a una celda inexistente.
    vector<vector<int>> propios(k);
    for (int i = 0; i < n_points; i++) {
        int col = (ancho_celda > 0) ? (int)((x[i] - x_min) / ancho_celda) : 0;
        int fil = (alto_celda  > 0) ? (int)((y[i] - y_min) / alto_celda)  : 0;
        col = min(col, columnas - 1);
        fil = min(fil, filas - 1);
        propios[fil * columnas + col].push_back(i);
    }

    // candidatos[c]: puntos dentro del rectangulo de la celda c agrandado
    // `margen` por cada lado (la celda + su halo). Se usa un margen un poco
    // mayor que eps para que un redondeo de punto flotante en la orilla no
    // deje fuera a un vecino real; meter de mas un punto no cambia nada,
    // porque igual se revisa su distancia exacta contra eps.
    //
    // Cada celda arma su propia lista y no toca la de las demas, asi que
    // se puede hacer en paralelo sin condiciones de carrera.
    //
    // Ademas de los indices, se guarda una COPIA de las coordenadas de los
    // candidatos en arreglos contiguos (cand_x, cand_y). Asi el ciclo
    // interno del Paso 1 recorre memoria en orden, como en la serial, y el
    // compilador lo puede vectorizar. Con x[candidatos[c][b]] (acceso
    // indirecto) cada lectura salta a una posicion distinta de memoria y la
    // version con k = 2 y 1 hilo resultaba MAS lenta que la serial.
    double margen = eps * 1.000001 + 1e-12;
    vector<vector<int>> candidatos(k);
    vector<vector<double>> cand_x(k), cand_y(k);
    #pragma omp parallel for schedule(dynamic, 1)
    for (int c = 0; c < k; c++) {
        int fil = c / columnas, col = c % columnas;
        double cx0 = x_min + col * ancho_celda - margen;
        double cx1 = x_min + (col + 1) * ancho_celda + margen;
        double cy0 = y_min + fil * alto_celda - margen;
        double cy1 = y_min + (fil + 1) * alto_celda + margen;
        for (int j = 0; j < n_points; j++) {
            if (x[j] >= cx0 && x[j] <= cx1 && y[j] >= cy0 && y[j] <= cy1) {
                candidatos[c].push_back(j);
                cand_x[c].push_back(x[j]);
                cand_y[c].push_back(y[j]);
            }
        }
    }

    // --- Paso 1: CORE de primer orden, cada particion en paralelo ---
    // El "parallel for" reparte las k particiones entre los hilos. Dentro
    // de una particion, cada punto propio i se compara solo contra los
    // candidatos de su celda (no contra los n_points), y solo se escribe
    // es_core[i] para puntos propios: como cada punto es propio de UNA sola
    // celda, ningun par de hilos escribe la misma posicion -> sin carreras.
    //
    // schedule(dynamic, 1): las celdas tienen cantidades MUY distintas de
    // puntos (los blobs no estan repartidos parejo; hay celdas casi vacias),
    // asi que cada hilo toma una celda a la vez y pide otra al terminar.
    #pragma omp parallel for schedule(dynamic, 1)
    for (int c = 0; c < k; c++) {
        const vector<int>& mios = propios[c];
        const double* cx = cand_x[c].data();
        const double* cy = cand_y[c].data();
        int n_cand = (int)cand_x[c].size();
        for (int a = 0; a < (int)mios.size(); a++) {
            int i = mios[a];
            int vecinos = 0;
            // Misma formula que distancia(), pero contra la copia contigua.
            for (int b = 0; b < n_cand; b++) {
                double dx = x[i] - cx[b];
                double dy = y[i] - cy[b];
                if (sqrt(dx * dx + dy * dy) <= eps) {
                    vecinos++;
                }
            }
            if (vecinos >= min_samples) {
                es_core[i] = true;
            }
        }
    }
    // BARRERA implicita al terminar el parallel for: el Paso 2 de una celda
    // puede necesitar saber si un punto del halo (que pertenece a OTRA
    // celda) es core, asi que TODAS las celdas deben terminar el Paso 1.

    // --- Paso 2: core de segundo orden, cada particion en paralelo ---
    // Igual que en la serial: se lee de una COPIA fija del Paso 1 y se
    // escribe en es_core, para que un core de segundo orden no "rescate"
    // a otro punto (ni dentro de la celda ni a traves del halo).
    bool* es_core_paso1 = new bool[n_points];
    for (int i = 0; i < n_points; i++) es_core_paso1[i] = es_core[i];

    #pragma omp parallel for schedule(dynamic, 1)
    for (int c = 0; c < k; c++) {
        const vector<int>& mios = propios[c];
        const vector<int>& cand = candidatos[c];
        for (int a = 0; a < (int)mios.size(); a++) {
            int i = mios[a];
            if (es_core_paso1[i]) continue; // ya era core desde el Paso 1

            for (int b = 0; b < (int)cand.size(); b++) {
                int j = cand[b];
                if (es_core_paso1[j] && distancia(x, y, i, j) <= eps) {
                    es_core[i] = true;
                    break; // con un solo core cercano ya basta
                }
            }
        }
    }

    delete[] es_core_paso1;

    double fin = omp_get_wtime();
    double segundos = fin - inicio;

    // --- Escribir resultados (mismo formato que la serial) ---
    // No hay que "juntar" nada: es_core ya tiene la etiqueta de todos los
    // puntos, en el orden original del archivo de entrada.
    ofstream salida(ruta_salida);
    for (int i = 0; i < n_points; i++) {
        salida << x[i] << "," << y[i] << "," << (es_core[i] ? 1 : 0) << "\n";
    }
    salida.close();

    int total_ruido = 0;
    for (int i = 0; i < n_points; i++) {
        if (!es_core[i]) total_ruido++;
    }
    cerr << "n_points=" << n_points
         << " eps=" << eps
         << " min_samples=" << min_samples
         << " hilos=" << num_hilos
         << " k=" << k
         << " tiempo_segundos=" << segundos
         << " puntos_ruido=" << total_ruido << "\n";

    // --- Liberar memoria ---
    delete[] x;
    delete[] y;
    delete[] es_core;

    return 0;
}
