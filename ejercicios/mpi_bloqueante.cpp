// Ejercicio MPI con interaccion bloqueante (MPI.pdf, p17-p20).
// Los procesos se emparejan: el proceso i de la primera mitad habla con i + n/2.
// Con 8 procesos: 0<->4, 1<->5, 2<->6, 3<->7 (las flechas de la p20).
// Cada uno manda su id a su pareja y recibe el id de ella, con MPI_Send / MPI_Recv.
//
// Compilar:  mpic++ -std=c++17 -O2 mpi_bloqueante.cpp -o mpi_bloqueante.out
// Correr:    mpirun -n 8 ./mpi_bloqueante.out      (el numero de procesos debe ser par)
#include <mpi.h>
#include <iostream>
using namespace std;

int main(int argc, char** argv) {
    int process_id, num_processes;
    int partner;    // con quien me toca hablar
    int message;    // aqui llega el id de mi pareja
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    if (num_processes % 2 != 0) {
        if (process_id == 0)
            cout << "Se necesita un numero par de procesos (p. ej. mpirun -n 8)." << endl;
        MPI_Finalize();
        return 1;
    }

    int mitad = num_processes / 2;
    if (process_id < mitad) {
        // Primera mitad: PRIMERO envia (tag 1) y DESPUES recibe (tag 2).
        partner = process_id + mitad;
        MPI_Send(&process_id, 1, MPI_INT, partner, 1, MPI_COMM_WORLD);
        MPI_Recv(&message, 1, MPI_INT, partner, 2, MPI_COMM_WORLD, &status);
    } else {
        // Segunda mitad: el orden contrario. PRIMERO recibe (tag 1) y DESPUES envia (tag 2).
        // Si los dos lados empezaran con MPI_Recv, los dos se quedarian esperando para
        // siempre (deadlock).
        partner = process_id - mitad;
        MPI_Recv(&message, 1, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);
        MPI_Send(&process_id, 1, MPI_INT, partner, 2, MPI_COMM_WORLD);
    }

    cout << "Proceso " << process_id << ": mi pareja es " << partner
         << " y me mando el mensaje " << message << endl;

    MPI_Finalize();
    return 0;
}
