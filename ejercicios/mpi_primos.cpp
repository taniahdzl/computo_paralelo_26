// Ejercicio MPI: contar los numeros primos en [2, 10^n], n = 1..N (MPI.pdf, p30-p38).
//   - MPI_Bcast:  el proceso 0 le avisa a todos el limite del rango (10, 100, 1000, ...).
//   - Reparto ciclico (p36): el proceso id revisa 2+id, 2+id+P, 2+id+2P, ... (P = num. de procesos).
//   - Cada proceso cuenta SUS primos y MPI_Reduce con MPI_SUM los junta en el proceso 0.
//   - El proceso 0 mide el tiempo con MPI_Wtime (p32) y corre la version serial para el speedup.
// Imprime: limite  primos_paralelo  primos_serial  t_serial  t_paralelo  speedup
//
// Compilar:  mpic++ -std=c++17 -O2 mpi_primos.cpp -o mpi_primos.out
// Correr:    mpirun -n 3 ./mpi_primos.out [N]     (N = exponente maximo, default 8; maximo 9)
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

// Version serial (la de primos_serial.cpp): un solo proceso revisa todo [2, limite].
// La corre solo el proceso 0 como linea base para calcular el speedup.
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
    if (max_exponente < 1 || max_exponente > 9) max_exponente = 8;  // 10^9 aun cabe en int

    if (id == 0) cout << "N  primos_paralelo  primos_serial  t_serial  t_paralelo  speedup" << endl;

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
