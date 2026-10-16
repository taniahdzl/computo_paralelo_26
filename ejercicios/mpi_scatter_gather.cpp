// Rutinas de comunicacion colectiva (MPI.pdf, p15, p26-p29), con los datos de la p27.
// El proceso 0 tiene una matriz de 3 vectores de 5 numeros:
//   1. MPI_Bcast:   el proceso 0 avisa a todos cuantos elementos tiene cada vector.
//   2. MPI_Scatter: reparte un vector (renglon) a cada proceso.
//   3. Cada proceso calcula el promedio de SU vector.
//   4. MPI_Gather:  el proceso 0 junta los 3 promedios en un arreglo.
//   5. MPI_Reduce:  el proceso 0 recibe la suma (MPI_SUM) y el maximo (MPI_MAX) de los promedios.
//
// Compilar:  mpic++ -std=c++17 -O2 mpi_scatter_gather.cpp -o mpi_scatter_gather.out
// Correr:    mpirun -n 3 ./mpi_scatter_gather.out     (exactamente 3: uno por vector)
#include <mpi.h>
#include <iostream>
using namespace std;

const int NUM_PROCESSES = 3;   // NUM_VECTORS: un vector por proceso
const int VECTOR_SIZE = 5;
const int ROOT = 0;            // el proceso que reparte y junta

int main(int argc, char** argv) {
    int process_id, num_processes;

    // Solo el proceso 0 necesita los datos completos, pero en C++ es mas simple que todos
    // declaren el arreglo; MPI_Scatter solo lee el de ROOT.
    float data[NUM_PROCESSES][VECTOR_SIZE] = {{1.0, 2.0, 3.0, 4.0, 5.0},
                                              {6.0, 7.0, 8.0, 9.0, 10.0},
                                              {11.0, 12.0, 13.0, 14.0, 15.0}};
    float message[VECTOR_SIZE];                  // aqui llega MI vector
    float average = 0.0;                         // promedio de MI vector
    float data_to_be_collected[NUM_PROCESSES];   // en ROOT: los promedios de todos
    float suma_promedios = 0.0, max_promedio = 0.0;
    int vector_size = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    if (num_processes != NUM_PROCESSES) {
        if (process_id == ROOT)
            cout << "Correr con exactamente " << NUM_PROCESSES << " procesos (mpirun -n 3)." << endl;
        MPI_Finalize();
        return 1;
    }

    // 1. Broadcast: ROOT lo escribe y todos lo reciben. TODOS llaman a MPI_Bcast
    //    (no hay un "MPI_Recv" del otro lado): asi funcionan las colectivas.
    if (process_id == ROOT) vector_size = VECTOR_SIZE;
    MPI_Bcast(&vector_size, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

    // 2. Scatter: el renglon i de data va al proceso i. "VECTOR_SIZE" aparece dos veces:
    //    cuantos elementos se envian A CADA proceso y cuantos recibe cada uno.
    MPI_Scatter(data, VECTOR_SIZE, MPI_FLOAT,
                message, VECTOR_SIZE, MPI_FLOAT,
                ROOT, MPI_COMM_WORLD);

    // 3. Trabajo local (sin comunicacion).
    for (int i = 0; i < vector_size; i++) average += message[i];
    average /= vector_size;
    cout << "Proceso " << process_id << ": recibi [" << message[0] << " .. "
         << message[vector_size - 1] << "], promedio = " << average << endl;

    // 4. Gather: cada proceso manda 1 float; ROOT recibe 1 de cada uno, en orden de rank.
    MPI_Gather(&average, 1, MPI_FLOAT,
               data_to_be_collected, 1, MPI_FLOAT,
               ROOT, MPI_COMM_WORLD);

    // 5. Reduce: combina los valores de todos con una operacion (p29).
    MPI_Reduce(&average, &suma_promedios, 1, MPI_FLOAT, MPI_SUM, ROOT, MPI_COMM_WORLD);
    MPI_Reduce(&average, &max_promedio,   1, MPI_FLOAT, MPI_MAX, ROOT, MPI_COMM_WORLD);

    if (process_id == ROOT) {
        cout << "ROOT junto los promedios (Gather):";
        for (int i = 0; i < NUM_PROCESSES; i++) cout << " " << data_to_be_collected[i];
        cout << endl;
        cout << "ROOT: suma de promedios (Reduce MPI_SUM) = " << suma_promedios
             << ", maximo (Reduce MPI_MAX) = " << max_promedio << endl;
    }

    MPI_Finalize();
    return 0;
}
