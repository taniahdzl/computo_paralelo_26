"""Genera <n>_data.csv igual que la celda 1 de notebooks/DBSCAN_noise.ipynb.

Uso:
    .venv/bin/python generar_datos.py <carpeta_salida> <n1> [<n2> ...]

Con --sklearn ademas escribe <n>_sklearn.csv (x,y,label) con el resultado del
DBSCAN de sklearn (eps=0.03, min_samples=10), para validar nuestras versiones.
"""
import sys
from pathlib import Path

import numpy as np
from sklearn.datasets import make_blobs


def generar(n_points, carpeta, con_sklearn=False):
    # Mismos parametros que el notebook del curso.
    points, _ = make_blobs(n_samples=n_points,
                           centers=4,
                           cluster_std=0.06,
                           random_state=11,
                           center_box=(0, 1.0))
    points = np.round(np.abs(points[:, ::-1]), 3)
    ruta = carpeta / f"{n_points}_data.csv"
    np.savetxt(ruta, points, delimiter=",", fmt="%.3f")

    if con_sklearn:
        from sklearn.cluster import DBSCAN
        clusters = DBSCAN(eps=0.03, min_samples=10).fit_predict(points)
        etiquetas = np.where(clusters == -1, 0, 1)
        np.savetxt(carpeta / f"{n_points}_sklearn.csv",
                   np.column_stack([points, etiquetas]),
                   delimiter=",", fmt=["%.3f", "%.3f", "%d"])
    return ruta


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if a != "--sklearn"]
    con_sklearn = "--sklearn" in sys.argv
    carpeta = Path(args[0])
    carpeta.mkdir(parents=True, exist_ok=True)
    for n in args[1:]:
        print(generar(int(n), carpeta, con_sklearn))
