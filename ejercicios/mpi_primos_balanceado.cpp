// Compilar:  mpic++ -std=c++17 -O2 mpi_primos_balanceado.cpp -o mpi_primos_balanceado.out
// Correr:    mpirun -n 8 ./mpi_primos_balanceado.out [N]
#include <mpi.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace std;

bool esPrimo(int possible_prime) {
    int limite =(int)trunc(sqrt((double)possible_prime));
    for (int divisor = 2; divisor <= limite; divisor++)
        if (possible_prime % divisor == 0) return false;
    return true;
}

// Version serial (la de primos_serial.cpp): un solo proceso revisa todo [2, limite].
int contarPrimosSerial(int limite) {
    int primos = 0;
    for (int possible_prime = 2; possible_prime <= limite; possible_prime++)
        if (esPrimo(possible_prime)) primos++;
    return primos;
}

int main(int argc, char** argv) {
    int id, num_processes;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    int max_exponente = (argc > 1) ? atoi(argv[1]) : 8;
    if (max_exponente < 1 || max_exponente > 9) max_exponente = 8;

    if (id == 0) cout << "N  primos_paralelo  primos_serial  t_serial  t_paralelo  speedup" << endl;

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
            double tiempo_paralelo = end - start;

            // Fuera de la medicion paralela; los demas procesos esperan en el siguiente MPI_Bcast.
            double start_serial = MPI_Wtime();
            int primos_serial = contarPrimosSerial(limite);
            double tiempo_serial = MPI_Wtime() - start_serial;

            cout << limite << "  " << primos_totales << "  " << primos_serial << "  "
                 << tiempo_serial << "  " << tiempo_paralelo << "  "
                 << tiempo_serial / tiempo_paralelo << endl;
        }
    }

    MPI_Finalize();
    return 0;
}
