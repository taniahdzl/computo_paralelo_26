// Ejercicio MPI: contar los numeros primos en [2, 10^n], n = 1..N (MPI.pdf, p30-p38).
//   - MPI_Bcast:  el proceso 0 le avisa a todos el limite del rango (10, 100, 1000, ...).
//   - Reparto ciclico (p36): el proceso id revisa 2+id, 2+id+P, 2+id+2P, ... (P = num. de procesos).
//   - Cada proceso cuenta SUS primos y MPI_Reduce con MPI_SUM los junta en el proceso 0.
//   - El proceso 0 mide el tiempo con MPI_Wtime (p32).
// Imprime, como en la p31:  limite  cantidad_de_primos  segundos
//
// Compilar:  mpic++ -std=c++17 -O2 mpi_primos.cpp -o mpi_primos.out
// Correr:    mpirun -n 8 ./mpi_primos.out [N]     (N = exponente maximo, default 8; maximo 9)
#include <mpi.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace std;

// Prueba de primalidad de la p35: buscar un divisor de 2 a sqrt(possible_prime).
// Si n = a*b con a <= b, entonces a <= sqrt(n): basta con revisar hasta la raiz.
bool esPrimo(int possible_prime) {
    int limite = (int)trunc(sqrt((double)possible_prime));
    for (int divisor = 2; divisor <= limite; divisor++)
        if (possible_prime % divisor == 0) return false;
    return true;
}

int main(int argc, char** argv) {
    int id, num_processes;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    int max_exponente = (argc > 1) ? atoi(argv[1]) : 8;
    if (max_exponente < 1 || max_exponente > 9) max_exponente = 8;  // 10^9 aun cabe en int

    int limite = 1;
    for (int n = 1; n <= max_exponente; n++) {
        double start = 0.0, end = 0.0;
        if (id == 0) {
            limite *= 10;
            start = MPI_Wtime();
        }

        // Todos llaman a MPI_Bcast; al regresar, todos tienen el limite del proceso 0.
        MPI_Bcast(&limite, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Reparto ciclico: con 4 procesos, el 0 revisa 2, 6, 10, ...; el 1 revisa 3, 7, 11, ...
        int primos_locales = 0;
        for (int possible_prime = 2 + id; possible_prime <= limite; possible_prime += num_processes)
            if (esPrimo(possible_prime)) primos_locales++;

        // Suma de los conteos de todos; solo el proceso 0 recibe el total.
        int primos_totales = 0;
        MPI_Reduce(&primos_locales, &primos_totales, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        if (id == 0) {
            end = MPI_Wtime();
            cout << limite << "  " << primos_totales << "  " << end - start << endl;
        }
    }

    MPI_Finalize();
    return 0;
}
