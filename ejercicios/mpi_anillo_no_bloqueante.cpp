// Ejercicio MPI con interaccion NO bloqueante (MPI.pdf, p22-p25).
// Los procesos forman un anillo: 0 - 1 - 2 - ... - n-1 - 0.
// Cada proceso manda su id a sus DOS vecinos (izquierdo y derecho) y recibe el id de ambos,
// con MPI_Isend / MPI_Irecv, y luego espera a que todo termine con MPI_Wait.
//
// Compilar:  mpic++ -std=c++17 -O2 mpi_anillo_no_bloqueante.cpp -o mpi_anillo_no_bloqueante.out
// Correr:    mpirun -n 6 ./mpi_anillo_no_bloqueante.out
#include <mpi.h>
#include <iostream>
using namespace std;

int main(int argc, char** argv) {
    int process_id, num_processes;
    int recibido_izq, recibido_der;   // buffers de recepcion (uno por vecino)
    MPI_Request requests[4];          // un "recibo" por cada operacion pendiente
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    // Vecinos en el anillo. El "+ num_processes" evita el modulo de un numero negativo
    // cuando process_id = 0 (su vecino izquierdo es el ultimo proceso).
    int izq = (process_id - 1 + num_processes) % num_processes;
    int der = (process_id + 1) % num_processes;

    // Las 4 llamadas regresan de inmediato: solo "registran" la operacion.
    // Por eso el orden no importa y no hay deadlock, aunque todos empiecen recibiendo.
    // Tag 1 = "viaja hacia la derecha", tag 2 = "viaja hacia la izquierda".
    MPI_Irecv(&recibido_izq, 1, MPI_INT, izq, 1, MPI_COMM_WORLD, &requests[0]);
    MPI_Irecv(&recibido_der, 1, MPI_INT, der, 2, MPI_COMM_WORLD, &requests[1]);
    MPI_Isend(&process_id,   1, MPI_INT, der, 1, MPI_COMM_WORLD, &requests[2]);
    MPI_Isend(&process_id,   1, MPI_INT, izq, 2, MPI_COMM_WORLD, &requests[3]);

    // Aqui se podria hacer trabajo util mientras los mensajes viajan (traslapar
    // comunicacion y computo); es la ventaja de lo no bloqueante.

    // Antes de LEER los buffers de recepcion (o de reusar process_id) hay que esperar.
    for (int r = 0; r < 4; r++)
        MPI_Wait(&requests[r], &status);

    cout << "Proceso " << process_id << ": vecino izq " << izq << " me mando " << recibido_izq
         << ", vecino der " << der << " me mando " << recibido_der << endl;

    MPI_Finalize();
    return 0;
}
