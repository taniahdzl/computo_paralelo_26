// Funciones compartidas por la version serial y la version MPI del Proyecto Clausura.
// Estan en un solo lugar para que las dos versiones cuenten y escriban EXACTAMENTE igual
// (asi el CSV serial y el CSV paralelo son comparables).
//
// Regla de limpieza (debe coincidir con data/validar_sklearn.py): se pasa a minusculas y una
// palabra es una secuencia de 2 o mas caracteres a-z o 0-9 (solo ASCII). Cualquier otro byte
// (espacios, signos, '_', letras con acento) separa palabras. No se quitan palabras vacias.
//
// Descarga: se usa curl via std::system y se lee el archivo temporal libro_<i>.txt (i = indice
// global del libro, para que en MPI dos procesos nunca escriban el mismo archivo).
#pragma once

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Lee el listado de URLs: una por linea. Se ignoran las lineas vacias y las que empiezan con '#'.
inline vector<string> leerUrls(const string& ruta) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el listado de URLs: " << ruta << "\n";
        exit(1);
    }
    vector<string> urls;
    string linea;
    while (getline(archivo, linea)) {
        // quita espacios al principio y al final
        size_t ini = linea.find_first_not_of(" \t\r\n");
        if (ini == string::npos) continue;
        size_t fin = linea.find_last_not_of(" \t\r\n");
        linea = linea.substr(ini, fin - ini + 1);
        if (linea[0] == '#') continue;
        urls.push_back(linea);
    }
    return urls;
}

// Lee un archivo completo a un string.
inline string leerArchivo(const string& ruta) {
    ifstream archivo(ruta, ios::binary);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo: " << ruta << "\n";
        exit(1);
    }
    stringstream ss;
    ss << archivo.rdbuf();
    return ss.str();
}

// Devuelve el texto completo del libro. Si la fuente empieza con "http" se descarga con curl a
// libro_<indice>.txt; si no, se trata de una ruta local (sirve para probar sin internet).
inline string obtenerTexto(const string& fuente, int indice) {
    if (fuente.rfind("http", 0) != 0) return leerArchivo(fuente);

    // La URL se pasa a un shell: solo se aceptan caracteres seguros para evitar inyeccion.
    for (char c : fuente) {
        unsigned char u = (unsigned char)c;
        if (!(isalnum(u) || c == ':' || c == '/' || c == '.' || c == '-' || c == '_' || c == '~' ||
              c == '%' || c == '?' || c == '=' || c == '&')) {
            cerr << "URL con caracteres no permitidos: " << fuente << "\n";
            exit(1);
        }
    }
    string temporal = "libro_" + to_string(indice) + ".txt";
    string orden = "curl -sSL --fail -o " + temporal + " " + fuente;
    if (system(orden.c_str()) != 0) {
        cerr << "Fallo la descarga de " << fuente << "\n";
        exit(1);
    }
    string texto = leerArchivo(temporal);
    remove(temporal.c_str());
    return texto;
}

// Limpia el texto y cuenta cuantas veces aparece cada palabra (regla del encabezado del archivo).
// Se recorre el texto una vez: se acumulan los caracteres a-z/0-9 (en minuscula) y al encontrar
// cualquier otro byte se cierra la palabra; si tiene 2 o mas caracteres se cuenta.
inline map<string, int> contarPalabras(const string& texto) {
    map<string, int> conteo;
    string palabra;
    auto cerrar = [&]() {
        if (palabra.size() >= 2) conteo[palabra]++;
        palabra.clear();
    };
    for (unsigned char c : texto) {
        if (c < 128 && isalnum(c)) palabra += (char)tolower(c);
        else cerrar();
    }
    cerrar();  // por si el texto termina en medio de una palabra
    return conteo;
}

// Vocabulario: la union de las palabras de todos los libros. Un set de strings queda
// ordenado alfabeticamente, igual que las columnas de la libreta.
inline vector<string> construirVocabulario(const vector<map<string, int>>& conteos) {
    set<string> unicas;
    for (const auto& c : conteos)
        for (const auto& par : c) unicas.insert(par.first);
    return vector<string>(unicas.begin(), unicas.end());
}

// Escribe el CSV: primera fila = palabras; despues una fila por libro con sus conteos.
inline void escribirCsv(const string& ruta, const vector<string>& vocabulario,
                 const vector<map<string, int>>& conteos) {
    ofstream salida(ruta);
    if (!salida.is_open()) {
        cerr << "No se pudo crear el archivo de salida: " << ruta << "\n";
        exit(1);
    }
    for (size_t j = 0; j < vocabulario.size(); j++) {
        if (j > 0) salida << ",";
        salida << vocabulario[j];
    }
    salida << "\n";
    for (const auto& c : conteos) {
        for (size_t j = 0; j < vocabulario.size(); j++) {
            if (j > 0) salida << ",";
            auto it = c.find(vocabulario[j]);
            salida << (it == c.end() ? 0 : it->second);
        }
        salida << "\n";
    }
}
