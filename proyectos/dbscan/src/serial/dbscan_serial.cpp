// dbscan_serial.cpp
//
// Implementacion serial (no paralela) de un DBSCAN "ingenuo" para detectar
// puntos de ruido/outliers, siguiendo el algoritmo de 2 pasos visto en clase:
//
//   Paso 1: clasificar cada punto como CORE (de primer orden) si tiene al
//           menos `min_samples` puntos (incluyendose a si mismo) a una
//           distancia euclidiana <= eps. Si no, se clasifica como RUIDO.
//
//   Paso 2: para cada punto que quedo como RUIDO, revisar si esta a
//           distancia <= eps de algun punto CORE. Si es asi, se reclasifica
//           como CORE (de segundo orden) -- es "epsilon-alcanzable" desde
//           una zona densa aunque el mismo no tenga suficientes vecinos.
//
// Nota: este proyecto solo pide distinguir RUIDO vs NO-RUIDO (no asignar
// un identificador de cluster a cada punto), asi que no hace falta
// propagar un "cluster id" -- basta un booleano por punto.
//
// Entrada:  un CSV sin encabezado, con columnas x,y (el que genera la
//           celda 1 de DBSCAN_noise.ipynb, p.ej. "4000_data.csv").
// Salida:   un CSV sin encabezado, con columnas x,y,label, donde
//           label = 0 si el punto es ruido, 1 si es core
//           (formato que espera la celda 2 de DBSCAN_noise.ipynb).
//
// Uso:
//   g++ -fopenmp -O2 dbscan_serial.cpp -o dbscan_serial
//   ./dbscan_serial 4000_data.csv 4000_results.csv 0.03 10

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <omp.h>
#include <cstdlib>

using namespace std;

// Lee un CSV de dos columnas (x,y) y reserva arreglos dinamicos para
// almacenar las coordenadas. Devuelve el numero de puntos leidos por
// referencia en n_points.
void leerPuntos(const string& ruta, double*& x, double*& y, int& n_points) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo de entrada: " << ruta << "\n";
        exit(1);
    }

    // Primera pasada: contar cuantas lineas (puntos) hay, para saber
    // cuanta memoria reservar de una sola vez.
    string linea;
    n_points = 0;
    while (getline(archivo, linea)) {
        if (!linea.empty()) n_points++;
    }

    // Reservar los arreglos dinamicos ya con el tamano exacto.
    x = new double[n_points];
    y = new double[n_points];

    // Segunda pasada: releer el archivo desde el inicio y parsear cada
    // linea "x,y" separando por la coma.
    archivo.clear();               // limpiar el flag de fin de archivo
    archivo.seekg(0, ios::beg);    // regresar el cursor al inicio

    int i = 0;
    while (getline(archivo, linea)) {
        if (linea.empty()) continue;
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

int main(int argc, char** argv) {

    if (argc != 5) {
        cout << "Uso: " << argv[0]
             << " <entrada.csv> <salida.csv> <eps> <min_samples>\n";
        return 1;
    }

    string ruta_entrada  = argv[1];
    string ruta_salida   = argv[2];
    double eps           = atof(argv[3]);
    int min_samples      = atoi(argv[4]);

    // --- Cargar los puntos ---
    double* x = nullptr;
    double* y = nullptr;
    int n_points = 0;
    leerPuntos(ruta_entrada, x, y, n_points);

    // es_core[i] == true  -> el punto i es core (de 1er o 2do orden)
    // es_core[i] == false -> el punto i es ruido/outlier
    bool* es_core = new bool[n_points];
    for (int i = 0; i < n_points; i++) es_core[i] = false;

    // A partir de aqui empieza el trabajo que de verdad queremos medir
    // (lo que luego se va a paralelizar); por eso el cronometro arranca
    // justo antes del Paso 1 y no incluye la lectura del archivo.
    // Usamos omp_get_wtime() (en vez de otro reloj de C++) para medir
    // exactamente igual en esta version y en las dos versiones paralelas,
    // y asi poder comparar tiempos de forma justa.
    double inicio = omp_get_wtime();

    // --- Paso 1: encontrar los puntos CORE de primer orden ---
    // Para cada punto i, contamos cuantos puntos (incluido el mismo) caen
    // dentro de un radio `eps`. Si ese conteo alcanza min_samples, i es core.
    //
    // Complejidad: por cada uno de los n_points puntos, recorremos los
    // n_points puntos -> O(n^2). Esto es exactamente lo que se va a
    // paralelizar mas adelante, porque cada iteracion de "i" es
    // independiente de las demas (no hay dependencias de datos entre ellas).
    for (int i = 0; i < n_points; i++) {
        int vecinos = 0;
        for (int j = 0; j < n_points; j++) {
            if (distancia(x, y, i, j) <= eps) {
                vecinos++;
            }
        }
        if (vecinos >= min_samples) {
            es_core[i] = true;
        }
    }

    // --- Paso 2: reclasificar ruido epsilon-alcanzable (core de 2do orden) ---
    // Para cada punto que SIGUE siendo ruido despues del paso 1, revisamos
    // si hay algun punto core a distancia <= eps. Si lo hay, dejo de ser
    // ruido: se reclasifica como core (de segundo orden), aunque el mismo
    // no tenga min_samples vecinos.
    //
    // OJO -- bug clasico que hay que evitar aqui: si se lee Y se escribe el
    // MISMO arreglo es_core dentro de este for, un punto que se reclasifica
    // a core a la mitad del ciclo puede hacer que OTRO punto, revisado mas
    // adelante en el mismo for, lo vea como core y se encadene a el -- aunque
    // ese primer punto nunca haya sido core real del Paso 1. Eso provoca una
    // propagacion en cadena (como un BFS no intencional) que depende del
    // ORDEN en que se recorren los puntos, y clasifica como core puntos que
    // en realidad deberian seguir siendo ruido.
    //
    // La solucion es comparar siempre contra una COPIA fija del resultado
    // del Paso 1 (es_core_paso1), que nunca se modifica dentro de este for;
    // los cambios de esta etapa se escriben en es_core, pero se LEEN de
    // es_core_paso1. Asi el resultado no depende del orden de recorrido.
    // (Este mismo cuidado es el que mas adelante evita condiciones de
    // carrera al paralelizar este paso con OpenMP.)
    bool* es_core_paso1 = new bool[n_points];
    for (int i = 0; i < n_points; i++) es_core_paso1[i] = es_core[i];

    for (int i = 0; i < n_points; i++) {
        if (es_core_paso1[i]) continue; // ya era core desde el Paso 1

        for (int j = 0; j < n_points; j++) {
            if (es_core_paso1[j] && distancia(x, y, i, j) <= eps) {
                es_core[i] = true;
                break; // con un solo core cercano ya basta, no sigas buscando
            }
        }
    }

    delete[] es_core_paso1;

    double fin = omp_get_wtime();
    double segundos = fin - inicio;

    // --- Escribir resultados ---
    // Formato esperado por la celda 2 de DBSCAN_noise.ipynb: x,y,label
    // donde label = 1 (core / no ruido) o 0 (ruido), para que
    // plt.scatter(..., c=result.T[2]) pinte el ruido de un color distinto.
    ofstream salida(ruta_salida);
    for (int i = 0; i < n_points; i++) {
        salida << x[i] << "," << y[i] << "," << (es_core[i] ? 1 : 0) << "\n";
    }
    salida.close();

    // Reporte en pantalla (stderr para no mezclarse con el csv si algun dia
    // se redirige stdout): tiempo de ejecucion y cuantos puntos quedaron
    // como ruido, util para verificar que el resultado tiene sentido.
    int total_ruido = 0;
    for (int i = 0; i < n_points; i++) {
        if (!es_core[i]) total_ruido++;
    }
    cerr << "n_points=" << n_points
         << " eps=" << eps
         << " min_samples=" << min_samples
         << " tiempo_segundos=" << segundos
         << " puntos_ruido=" << total_ruido << "\n";

    // --- Liberar memoria ---
    delete[] x;
    delete[] y;
    delete[] es_core;

    return 0;
}