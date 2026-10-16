// Variante de mpi_primos.cpp con mejor balance de carga (no viene en las diapositivas).
// Problema del reparto ciclico de la p36: con un numero PAR de procesos, el proceso id solo
// revisa numeros con la misma paridad que 2+id. Los de id par solo reciben numeros pares, que
// se descartan en la primera division: la mitad de los procesos casi no trabaja.
// Arreglo: el 2 se cuenta aparte y solo se reparten los IMPARES: 3+2*id, 3+2*id+2P, ...
//
// Compilar:  mpic++ -std=c++17 -O2 mpi_primos_balanceado.cpp -o mpi_primos_balanceado.out
// Correr:    mpirun -n 8 ./mpi_primos_balanceado.out [N]
#include <mpi.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace std;

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
    if (max_exponente < 1 || max_exponente > 9) max_exponente = 8;

    int limite = 1;
    for (int n = 1; n <= max_exponente; n++) {
        double start = 0.0, end = 0.0;
        if (id == 0) {
            limite *= 10;
            start = MPI_Wtime();
        }
        MPI_Bcast(&limite, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // El 2 es el unico primo par: lo cuenta solo el proceso 0.
        int primos_locales = (id == 0) ? 1 : 0;
        // Solo impares, repartidos ciclicamente: cada proceso recibe la misma mezcla de numeros.
        // long long: possible_prime + 2*num_processes podria pasarse de INT_MAX cerca de 10^9.
        for (long long possible_prime = 3 + 2LL * id; possible_prime <= limite;
             possible_prime += 2LL * num_processes)
            if (esPrimo((int)possible_prime)) primos_locales++;

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
