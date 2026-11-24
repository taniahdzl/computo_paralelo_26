"""Resume experiments/corridas_locales.txt (salida de bolsa_mpi con 5 repeticiones por q):
genera resultados_locales.csv (media, minimo y maximo del speedup por q) y speedup_locales.png.
Uso:  python3 experiments/analizar_corridas.py   (desde proyectos/bolsa-palabras)
"""
import csv
import re
import statistics as st
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

datos = defaultdict(lambda: {"total": [], "proc": [], "union": [], "t_ser": [], "t_par": []})
q = None
for linea in open("experiments/corridas_locales.txt"):
    m = re.match(r"q=(\d+) rep=\d+", linea)
    if m:
        q = int(m.group(1))
        continue
    nums = re.findall(r"[\d.]+(?:e-?\d+)?", linea.split(")", 1)[-1]) if ")" in linea else []
    if linea.startswith("procesamiento"):
        datos[q]["proc"].append(float(nums[2]))
    elif "union serial" in linea:
        datos[q]["union"].append(float(re.findall(r"([\d.]+) s", linea)[0]))
    elif linea.startswith("TOTAL"):
        datos[q]["t_ser"].append(float(nums[0]))
        datos[q]["t_par"].append(float(nums[1]))
        datos[q]["total"].append(float(nums[2]))

filas = []
for q in sorted(datos):
    d = datos[q]
    filas.append({
        "q": q,
        "reps": len(d["total"]),
        "t_serial_s": round(st.mean(d["t_ser"]), 4),
        "t_paralelo_s": round(st.mean(d["t_par"]), 4),
        "speedup_total_media": round(st.mean(d["total"]), 3),
        "speedup_total_min": round(min(d["total"]), 3),
        "speedup_total_max": round(max(d["total"]), 3),
        "speedup_procesamiento_media": round(st.mean(d["proc"]), 3),
        "union_serial_media_s": round(st.mean(d["union"]), 4),
    })
with open("experiments/resultados_locales.csv", "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(filas[0]))
    w.writeheader()
    w.writerows(filas)
for f in filas:
    print(f)

qs = [f["q"] for f in filas]
fig, ax = plt.subplots(figsize=(6, 4))
ax.errorbar(qs, [f["speedup_total_media"] for f in filas],
            yerr=[[f["speedup_total_media"] - f["speedup_total_min"] for f in filas],
                  [f["speedup_total_max"] - f["speedup_total_media"] for f in filas]],
            marker="o", capsize=3, label="speedup medido (media, min-max)")
ax.axhline(1.2, color="gray", linestyle="--", label="requisito: 1.2x")
ax.axhline(1.0, color="lightgray", linestyle=":")
ax.set_xlabel("procesos (q)")
ax.set_ylabel("speedup = T_serial / T_paralelo")
ax.set_title("Bolsa de palabras: 6 libros locales (sin descarga de red)")
ax.set_xticks(qs)
ax.legend()
fig.tight_layout()
fig.savefig("experiments/speedup_locales.png", dpi=150)
