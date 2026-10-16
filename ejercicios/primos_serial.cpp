// Version serial del ejercicio de primos (linea base para el speedup de mpi_primos.cpp).
// Misma prueba de primalidad; mide el tiempo con chrono como en la p33.
//
// Compilar:  clang++ -std=c++17 -O2 primos_serial.cpp -o primos_serial.out
// Correr:    ./primos_serial.out [N]     (N = exponente maximo, default 8; maximo 9)
#include <chrono>
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
    int max_exponente = (argc > 1) ? atoi(argv[1]) : 8;
    if (max_exponente < 1 || max_exponente > 9) max_exponente = 8;

    int limite = 1;
    for (int n = 1; n <= max_exponente; n++) {
        limite *= 10;
        auto start = chrono::high_resolution_clock::now();

        int primos = 0;
        for (int possible_prime = 2; possible_prime <= limite; possible_prime++)
            if (esPrimo(possible_prime)) primos++;

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
        cout << limite << "  " << primos << "  " << (float)duration.count() / 1000000 << endl;
    }
    return 0;
}
