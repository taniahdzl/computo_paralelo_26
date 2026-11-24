"""Genera la matriz de bolsa de palabras "esperada" con CountVectorizer (como la libreta de Colab)
y, opcionalmente, la compara contra el CSV que produce el programa en C++.

Uso:
    # generar la matriz esperada a partir de textos locales (un archivo por libro)
    python3 validar_sklearn.py libro1.txt libro2.txt --salida esperado.csv

    # compararla con la salida del programa
    python3 validar_sklearn.py libro1.txt libro2.txt --comparar salida_serial.csv

    # con palabras vacias en ingles (si deciden quitarlas)
    python3 validar_sklearn.py libro1.txt libro2.txt --stop-words english --comparar salida_serial.csv

Los libros deben ir en el mismo orden que las URLs de urls.txt.
"""
import argparse
import sys

import pandas as pd
from sklearn.feature_extraction.text import CountVectorizer


def main():
    p = argparse.ArgumentParser()
    p.add_argument("libros", nargs="+", help="archivos de texto, uno por libro")
    p.add_argument("--salida", help="donde guardar la matriz esperada (CSV)")
    p.add_argument("--comparar", help="CSV del programa en C++ para comparar")
    p.add_argument("--stop-words", default=None, choices=[None, "english"],
                   help="quitar palabras vacias (por defecto no)")
    args = p.parse_args()

    corpus = []
    for ruta in args.libros:
        with open(ruta, "r", encoding="utf-8", errors="ignore") as f:
            corpus.append(f.read())

    # Mismos parametros que la libreta (min_df=0.0, max_df=1.0; lowercase=True por defecto).
    # token_pattern fija la regla "solo ASCII": una palabra son 2 o mas caracteres a-z o 0-9.
    # El patron por defecto de sklearn tambien acepta '_' y letras con acento, y los textos de
    # Gutenberg usan _asi_ para las cursivas, asi que nunca coincidiria con un programa en C++
    # que solo usa isalnum sobre ASCII.
    cv = CountVectorizer(min_df=0.0, max_df=1.0, stop_words=args.stop_words,
                         token_pattern=r"[a-z0-9]{2,}")
    matriz = cv.fit_transform(corpus).toarray()
    esperado = pd.DataFrame(matriz, columns=cv.get_feature_names_out())
    print(f"Matriz esperada: {esperado.shape[0]} libros x {esperado.shape[1]} palabras")

    if args.salida:
        esperado.to_csv(args.salida, index=False)
        print(f"Guardada en {args.salida}")

    if args.comparar:
        try:
            obtenido = pd.read_csv(args.comparar, keep_default_na=False)
        except pd.errors.EmptyDataError:
            print("El CSV del programa esta vacio (todavia no cuenta palabras).")
            sys.exit(1)
        if list(obtenido.columns) != list(esperado.columns):
            falt = set(esperado.columns) - set(obtenido.columns)
            sobr = set(obtenido.columns) - set(esperado.columns)
            print(f"El vocabulario NO coincide. Faltan {len(falt)} palabras, sobran {len(sobr)}.")
            print("Ejemplos que faltan:", sorted(falt)[:10])
            print("Ejemplos que sobran:", sorted(sobr)[:10])
            sys.exit(1)
        if obtenido.shape != esperado.shape or not (obtenido.values == esperado.values).all():
            print("El vocabulario coincide, pero hay conteos distintos.")
            sys.exit(1)
        print("Las matrices son IDENTICAS.")


if __name__ == "__main__":
    main()
