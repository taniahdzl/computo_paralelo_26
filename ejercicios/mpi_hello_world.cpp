// Hola mundo con MPI (diapositivas MPI.pdf, p8-p17).
// Cada proceso imprime su id (rank), cuantos procesos hay y en que maquina corre.
//
// Compilar:  mpic++ -std=c++17 -O2 mpi_hello_world.cpp -o mpi_hello_world.out
// Correr:    mpirun -n 4 ./mpi_hello_world.out
#include <mpi.h>
#include <iostream>
using namespace std;

int main(int argc, char** argv) {
    int process_id;      // rank de este proceso: 0 .. num_processes-1
    int num_processes;   // cuantos procesos lanzo mpirun (-n)
    char host_name[MPI_MAX_PROCESSOR_NAME];  // MPI_MAX_PROCESSOR_NAME: tamano maximo del nombre
    int name_length;

    MPI_Init(&argc, &argv);                              // aqui empieza el codigo paralelo
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);          // quien soy
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);       // cuantos somos
    MPI_Get_processor_name(host_name, &name_length);     // en que maquina estoy

    cout << "Hola desde el proceso " << process_id << " de " << num_processes
         << " en " << host_name << endl;

    MPI_Finalize();                                      // aqui termina el codigo paralelo
    return 0;
}
