// Proyecto Clausura: Bolsa de palabras. VERSION SERIAL.
//
// Uso:  ./bolsa_serial <urls.txt> <salida.csv>
//
// Idea: para cada libro se cuenta cuantas veces aparece cada palabra (un map<string,int> por
// libro). Luego se arma el vocabulario (la union de todas las palabras, en orden alfabetico)
// y se escribe el CSV: una fila por libro y una columna por palabra.
// Las funciones estan en ../comun/bolsa_comun.h (las comparte con la version MPI).

#include <chrono>

#include "../comun/bolsa_comun.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "Uso: " << argv[0] << " <urls.txt> <salida.csv>\n";
        return 1;
    }
    string ruta_urls = argv[1];
    string ruta_salida = argv[2];

    vector<string> urls = leerUrls(ruta_urls);
    int k = (int)urls.size();

    // Se miden dos tiempos: el total (con descarga) y solo el procesamiento. Cual se reporta como
    // "el" tiempo es una decision por confirmar con el profesor (ver el README).
    auto t0 = chrono::steady_clock::now();
    vector<string> textos(k);
    for (int i = 0; i < k; i++) textos[i] = obtenerTexto(urls[i], i);

    auto t1 = chrono::steady_clock::now();
    vector<map<string, int>> conteos(k);
    for (int i = 0; i < k; i++) conteos[i] = contarPalabras(textos[i]);
    vector<string> vocabulario = construirVocabulario(conteos);
    escribirCsv(ruta_salida, vocabulario, conteos);
    auto t2 = chrono::steady_clock::now();

    double descarga = chrono::duration<double>(t1 - t0).count();
    double proceso = chrono::duration<double>(t2 - t1).count();
    cerr << "libros=" << k << " palabras_vocabulario=" << vocabulario.size()
         << " t_descarga=" << descarga << " t_procesamiento=" << proceso
         << " t_total=" << descarga + proceso << "\n";
    return 0;
}
