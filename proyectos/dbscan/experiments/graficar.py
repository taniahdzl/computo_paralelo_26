"""Genera las graficas de speedup a partir de resultados.csv.

    .venv/bin/python proyectos/dbscan/experiments/graficar.py

Escribe en experiments/graficas/:
  speedup_p1.png         speedup de P1 vs n, una linea por numero de hilos
  speedup_p2_k<k>.png    speedup de P2 vs n, una linea por numero de hilos
  speedup_p2_vs_k.png    speedup de P2 vs k con el n mas grande
  speedup_mejores.png    mejor P1 vs mejor P2 para cada n
"""
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

AQUI = Path(__file__).resolve().parent
GRAFICAS = AQUI / "graficas"


def lineas_por_hilos(df, titulo, ruta, x="n_points", xlabel="Número de puntos"):
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for h, g in df.groupby("hilos"):
        g = g.sort_values(x)
        ax.plot(g[x], g["speedup"], marker="o", label=f"{h} hilos")
    ax.axhline(1.5, color="gray", linestyle="--", linewidth=1,
               label="speedup mínimo pedido (1.5)")
    ax.set_title(titulo)
    ax.set_xlabel(xlabel)
    ax.set_ylabel("Speedup vs serial")
    ax.grid(alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(ruta, dpi=150)
    plt.close(fig)


def main():
    GRAFICAS.mkdir(exist_ok=True)
    df = pd.read_csv(AQUI / "resultados.csv")

    p1 = df[df.version == "p1"]
    lineas_por_hilos(p1, "P1 (matriz indivisible)", GRAFICAS / "speedup_p1.png")

    p2 = df[df.version == "p2"]
    for k, g in p2.groupby("k"):
        lineas_por_hilos(g, f"P2 (k = {k})", GRAFICAS / f"speedup_p2_k{k}.png")

    n_max = p2.n_points.max()
    g = p2[p2.n_points == n_max]
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for h, gh in g.groupby("hilos"):
        gh = gh.sort_values("k")
        ax.plot(gh["k"], gh["speedup"], marker="o", label=f"{h} hilos")
    ax.set_xscale("log", base=2)
    ax.set_title(f"P2: speedup vs k (n = {n_max})")
    ax.set_xlabel("k (particiones)")
    ax.set_ylabel("Speedup vs serial")
    ax.grid(alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(GRAFICAS / "speedup_p2_vs_k.png", dpi=150)
    plt.close(fig)

    mejores = (df[df.version != "serial"]
               .groupby(["version", "n_points"])["speedup"].max().unstack(0))
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for v in mejores.columns:
        ax.plot(mejores.index, mejores[v], marker="o", label=f"mejor {v.upper()}")
    ax.axhline(1.5, color="gray", linestyle="--", linewidth=1)
    ax.set_title("Mejor speedup de cada versión")
    ax.set_xlabel("Número de puntos")
    ax.set_ylabel("Speedup vs serial")
    ax.grid(alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(GRAFICAS / "speedup_mejores.png", dpi=150)
    plt.close(fig)
    print(f"Gráficas en {GRAFICAS}")


if __name__ == "__main__":
    main()
