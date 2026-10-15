// dbscan_p1.cpp
//
// Version paralela P1 del DBSCAN "ingenuo" de 2 pasos: se considera TODA
// la matriz de puntos como INDIVISIBLE. Cada punto se sigue comparando
// contra los n_points puntos (igual que en la serial); lo unico que cambia
// es que las iteraciones del for de afuera (el de "i") se reparten entre
// los hilos con OpenMP.
//
//   Paso 1: clasificar cada punto como CORE (de primer orden) si tiene al
//           menos `min_samples` puntos (incluyendose a si mismo) a una
//           distancia euclidiana <= eps. Si no, se clasifica como RUIDO.
//
//   Paso 2: para cada punto que quedo como RUIDO, revisar si esta a
//           distancia <= eps de algun punto CORE de primer orden. Si es asi,
//           se reclasifica como CORE (de segundo orden).
//
// El algoritmo, la entrada, la salida y la forma de medir el tiempo son
// IDENTICOS a dbscan_serial.cpp, para que el speedup sea una comparacion
// justa y para que las etiquetas salgan exactamente iguales.
//
// Entrada:  un CSV sin encabezado, con columnas x,y.
// Salida:   un CSV sin encabezado, con columnas x,y,label
//           (label = 0 ruido, 1 core).
//
// Uso (Mac con libomp de Homebrew):
//   clang++ -std=c++17 -O2 -Xpreprocessor -fopenmp -I/usr/local/opt/libomp/include \
//       -L/usr/local/opt/libomp/lib -lomp dbscan_p1.cpp -o dbscan_p1.out
//   ./dbscan_p1.out 4000_data.csv 4000_results.csv 0.03 10 8

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <omp.h>
#include <cstdlib>
#include <cctype>

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

int main(int argc, char** argv) {

    // Respecto a la serial se agrega un 5o argumento: el numero de hilos,
    // porque el experimento pide variarlo ({1, 8, 16, 32} en nuestra Mac).
    if (argc != 6) {
        cout << "Uso: " << argv[0]
             << " <entrada.csv> <salida.csv> <eps> <min_samples> <num_hilos>\n";
        return 1;
    }

    string ruta_entrada  = argv[1];
    string ruta_salida   = argv[2];
    double eps           = atof(argv[3]);
    int min_samples      = atoi(argv[4]);
    int num_hilos        = atoi(argv[5]);

    // Fija cuantos hilos usara cada region "#pragma omp parallel" que venga
    // despues (equivale a la variable de entorno OMP_NUM_THREADS).
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

    // Igual que en la serial: el cronometro solo mide los dos pasos, no la
    // lectura ni la escritura del CSV.
    double inicio = omp_get_wtime();

    // --- Paso 1: encontrar los puntos CORE de primer orden (en paralelo) ---
    // Cada iteracion de "i" es independiente de las demas:
    //   - solo LEE x[] y y[], que nadie modifica;
    //   - solo ESCRIBE es_core[i], y ningun otro hilo escribe esa posicion
    //     (a cada hilo le tocan valores de i distintos).
    // Por eso no hay condiciones de carrera y no hace falta critical/atomic.
    //
    // `vecinos` y `j` se declaran DENTRO del for, asi que cada hilo tiene su
    // propia copia (son privadas). La "i" del parallel for tambien es privada.
    //
    // schedule(static): todas las iteraciones cuestan lo mismo (cada una
    // recorre los n_points puntos), asi que basta con partir el rango de "i"
    // en bloques iguales, uno por hilo, sin el costo extra de repartir
    // trabajo dinamicamente.
    #pragma omp parallel for schedule(static)
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
    // Al terminar el "parallel for" hay una BARRERA implicita: ningun hilo
    // sigue hasta que todos terminaron el Paso 1. Esto es indispensable,
    // porque el Paso 2 necesita el resultado COMPLETO del Paso 1.

    // --- Paso 2: reclasificar ruido epsilon-alcanzable (en paralelo) ---
    // Igual que en la serial, se lee de una COPIA fija del Paso 1
    // (es_core_paso1) y se escribe en es_core. En paralelo esto es lo que
    // evita la condicion de carrera: si se leyera y escribiera es_core, el
    // resultado dependeria de que hilo llego primero (un punto recien
    // reclasificado podria "rescatar" a otro).
    bool* es_core_paso1 = new bool[n_points];
    for (int i = 0; i < n_points; i++) es_core_paso1[i] = es_core[i];

    // schedule(dynamic, 64): aqui las iteraciones NO cuestan lo mismo.
    //   - Si i ya era core, sale de inmediato (continue).
    //   - Si es ruido, el break corta en cuanto encuentra un core cercano,
    //     que puede ser al principio o al final del recorrido.
    // Con static, a un hilo le podria tocar un bloque con mucho ruido
    // aislado (que recorre los n_points completos) y a otro uno lleno de
    // cores (que no hace casi nada). Con dynamic, cada hilo pide un nuevo
    // bloque de 64 iteraciones cuando termina el suyo, y la carga se
    // balancea. Bloques de 64 (y no de 1) para no pagar el costo de
    // repartir trabajo en cada iteracion.
    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < n_points; i++) {
        if (es_core_paso1[i]) continue; // ya era core desde el Paso 1

        for (int j = 0; j < n_points; j++) {
            if (es_core_paso1[j] && distancia(x, y, i, j) <= eps) {
                es_core[i] = true;
                break; // con un solo core cercano ya basta
            }
        }
    }

    delete[] es_core_paso1;

    double fin = omp_get_wtime();
    double segundos = fin - inicio;

    // --- Escribir resultados (mismo formato que la serial) ---
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
         << " tiempo_segundos=" << segundos
         << " puntos_ruido=" << total_ruido << "\n";

    // --- Liberar memoria ---
    delete[] x;
    delete[] y;
    delete[] es_core;

    return 0;
}
