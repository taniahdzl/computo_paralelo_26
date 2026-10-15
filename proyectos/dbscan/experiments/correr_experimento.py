"""Corre el experimento completo del Proyecto Apertura (diapositiva 14).

    .venv/bin/python proyectos/dbscan/experiments/correr_experimento.py

- Compila serial, P1 y P2 con las mismas banderas (-O2).
- Genera <n>_data.csv para cada tamano con generar_datos.py.
- Corre 10 repeticiones de cada configuracion:
    serial:  n
    P1:      n x hilos
    P2:      n x hilos x k
- Guarda cada corrida en resultados_crudos.csv EN CUANTO TERMINA, asi que si
  se interrumpe (Ctrl+C, se duerme la laptop) basta con volver a correrlo:
  se salta las corridas que ya estan.
- En la primera repeticion de cada configuracion compara las etiquetas
  contra la serial; si alguna difiere, se detiene.
- Al final escribe resultados.csv con el promedio de las 10 repeticiones y
  el speedup contra la serial (mismo n).

Opciones:
    --rapido   solo n = 20000 y 2 repeticiones (para probar que todo funciona);
               escribe en prueba_*.csv, no en los resultados reales
"""
import csv
import os
import re
import subprocess
import sys
from pathlib import Path

from generar_datos import generar

AQUI = Path(__file__).resolve().parent
SRC = AQUI.parent / "src"
BIN = AQUI / "bin"
DATOS = AQUI / "datos"
CRUDOS = AQUI / "resultados_crudos.csv"
PROMEDIOS = AQUI / "resultados.csv"

EPS = "0.03"
MIN_SAMPLES = "10"

PUNTOS = [20000, 40000, 80000, 120000, 140000, 160000, 180000, 200000]
KS = [2, 4, 8, 16, 32, 64]
REPETICIONES = 10
VIRTUALES = os.cpu_count()  # 16 en la MacBook Pro i9 (8 fisicos x 2 HT)
HILOS = [1, VIRTUALES // 2, VIRTUALES, VIRTUALES * 2]

FUENTES = {
    "serial": SRC / "serial" / "dbscan_serial.cpp",
    "p1": SRC / "parallel" / "dbscan_p1.cpp",
    "p2": SRC / "parallel-2" / "dbscan_p2.cpp",
}
COMPILAR = ["clang++", "-std=c++17", "-O2", "-Xpreprocessor", "-fopenmp",
            "-I/usr/local/opt/libomp/include", "-L/usr/local/opt/libomp/lib",
            "-lomp"]


def compilar():
    BIN.mkdir(exist_ok=True)
    for nombre, fuente in FUENTES.items():
        subprocess.run(COMPILAR + [str(fuente), "-o", str(BIN / nombre)],
                       check=True)


def correr(version, n, hilos, k, salida):
    cmd = [str(BIN / version), str(DATOS / f"{n}_data.csv"), str(salida),
           EPS, MIN_SAMPLES]
    if version != "serial":
        cmd.append(str(hilos))
    if version == "p2":
        cmd.append(str(k))
    r = subprocess.run(cmd, capture_output=True, text=True, check=True)
    return float(re.search(r"tiempo_segundos=([0-9.eE+-]+)", r.stderr).group(1))


def etiquetas(ruta):
    with open(ruta) as f:
        return [linea.rsplit(",", 1)[1].strip() for linea in f]


def configuraciones(puntos):
    for n in puntos:
        yield ("serial", n, 1, 0)
        for h in HILOS:
            yield ("p1", n, h, 0)
        for h in HILOS:
            for k in KS:
                yield ("p2", n, h, k)


def main():
    global CRUDOS, PROMEDIOS
    rapido = "--rapido" in sys.argv
    if rapido:  # no mezclar la prueba con los resultados reales
        CRUDOS = AQUI / "prueba_crudos.csv"
        PROMEDIOS = AQUI / "prueba_promedios.csv"
    puntos = [20000] if rapido else PUNTOS
    reps = 2 if rapido else REPETICIONES

    compilar()
    DATOS.mkdir(exist_ok=True)
    for n in puntos:
        if not (DATOS / f"{n}_data.csv").exists():
            generar(n, DATOS)

    hechas = {}
    if CRUDOS.exists():
        with open(CRUDOS) as f:
            for fila in csv.DictReader(f):
                clave = (fila["version"], int(fila["n_points"]),
                         int(fila["hilos"]), int(fila["k"]))
                hechas[clave] = hechas.get(clave, 0) + 1

    nuevo = not CRUDOS.exists()
    with open(CRUDOS, "a", newline="") as f:
        w = csv.writer(f)
        if nuevo:
            w.writerow(["version", "n_points", "hilos", "k", "repeticion",
                        "tiempo_segundos"])
        for conf in configuraciones(puntos):
            version, n, h, k = conf
            ya = hechas.get(conf, 0)
            for rep in range(ya, reps):
                salida = DATOS / f"{n}_{version}.csv"
                t = correr(version, n, h, k, salida)
                if rep == 0 and version != "serial":
                    ref = DATOS / f"{n}_serial.csv"
                    if not ref.exists():
                        correr("serial", n, 1, 0, ref)
                    if etiquetas(salida) != etiquetas(ref):
                        sys.exit(f"ERROR: {conf} da etiquetas distintas a la serial")
                w.writerow([version, n, h, k, rep, f"{t:.6f}"])
                f.flush()
                print(f"{version:6} n={n:6} hilos={h:2} k={k:2} rep={rep} {t:.4f}s",
                      flush=True)

    promediar()


def promediar():
    suma, cuenta = {}, {}
    with open(CRUDOS) as f:
        for fila in csv.DictReader(f):
            clave = (fila["version"], int(fila["n_points"]),
                     int(fila["hilos"]), int(fila["k"]))
            suma[clave] = suma.get(clave, 0.0) + float(fila["tiempo_segundos"])
            cuenta[clave] = cuenta.get(clave, 0) + 1

    with open(PROMEDIOS, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["version", "n_points", "hilos", "k", "repeticiones",
                    "tiempo_promedio", "speedup"])
        for clave in sorted(suma, key=lambda c: (c[1], c[0], c[2], c[3])):
            version, n, h, k = clave
            prom = suma[clave] / cuenta[clave]
            serial = ("serial", n, 1, 0)
            speedup = (suma[serial] / cuenta[serial]) / prom if serial in suma else ""
            w.writerow([version, n, h, k, cuenta[clave], f"{prom:.6f}",
                        f"{speedup:.3f}" if speedup != "" else ""])
    print(f"Promedios en {PROMEDIOS}")


if __name__ == "__main__":
    main()
