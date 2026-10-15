"""Explica el speedup de P2 contando el trabajo de cada celda (sin cronometrar).

    .venv/bin/python proyectos/dbscan/experiments/analizar_celdas.py [n]

Arma las mismas celdas y halos que dbscan_p2.cpp para datos/<n>_data.csv
(n = 200000 por defecto) y, para cada k, imprime:

  reduccion    n^2 / (distancias del Paso 1 de P2). Es el speedup que P2
               deberia tener con 1 hilo solo por hacer menos trabajo.
  no_vacias    cuantas de las k celdas tienen al menos un punto propio.
  celda_max    % del trabajo total que hace la celda mas cargada.
  cota_8h      speedup maximo de repartir las celdas entre 8 hilos (cores
               fisicos): trabajo total / trabajo del hilo mas cargado,
               asignando cada celda, de la mas grande a la mas chica, al
               hilo menos cargado (parecido a lo que hace dynamic,1).
"""
import sys
from pathlib import Path

import numpy as np

AQUI = Path(__file__).resolve().parent
EPS = 0.03
HILOS = 8


def cuadricula(k):
    filas = 1
    while (filas * 2) * (filas * 2) <= k:
        filas *= 2
    return filas, k // filas


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 200000
    p = np.loadtxt(AQUI / "datos" / f"{n}_data.csv", delimiter=",")
    x, y = p[:, 0], p[:, 1]
    margen = EPS * 1.000001 + 1e-12

    print(f"n = {n}")
    print(" k  reduccion  no_vacias  celda_max  cota_8h")
    for k in [2, 4, 8, 16, 32, 64]:
        filas, columnas = cuadricula(k)
        ancho = (x.max() - x.min()) / columnas
        alto = (y.max() - y.min()) / filas
        col = np.minimum(((x - x.min()) / ancho).astype(int), columnas - 1)
        fil = np.minimum(((y - y.min()) / alto).astype(int), filas - 1)
        celda = fil * columnas + col

        trabajo = np.zeros(k)
        no_vacias = 0
        for c in range(k):
            f, cc = divmod(c, columnas)
            x0 = x.min() + cc * ancho - margen
            x1 = x.min() + (cc + 1) * ancho + margen
            y0 = y.min() + f * alto - margen
            y1 = y.min() + (f + 1) * alto + margen
            candidatos = ((x >= x0) & (x <= x1) & (y >= y0) & (y <= y1)).sum()
            propios = (celda == c).sum()
            trabajo[c] = propios * candidatos
            no_vacias += propios > 0

        cargas = np.zeros(HILOS)
        for t in sorted(trabajo, reverse=True):
            cargas[cargas.argmin()] += t
        total = trabajo.sum()
        print(f"{k:2}  {n * n / total:9.2f}  {no_vacias:9}  "
              f"{100 * trabajo.max() / total:8.1f}%  {total / cargas.max():7.2f}")


if __name__ == "__main__":
    main()
