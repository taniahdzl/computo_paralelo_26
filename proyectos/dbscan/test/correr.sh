#!/bin/bash
set -e
cd "$(dirname "$0")"

for v in serial/dbscan_serial:serial parallel/dbscan_p1:p1 parallel-2/dbscan_p2:p2; do
  clang++ -std=c++17 -O2 -Xpreprocessor -fopenmp -I/usr/local/opt/libomp/include \
    -L/usr/local/opt/libomp/lib -lomp ../src/${v%%:*}.cpp -o ${v#*:}.out
done

tiempo() { "$@" 2>&1 | sed -E 's/.*tiempo_segundos=([0-9.e-]+).*/\1/'; }
ts=$(tiempo ./serial.out 100000_data.csv serial.csv 0.03 10)
t1=$(tiempo ./p1.out 100000_data.csv p1.csv 0.03 10 4)
t2=$(tiempo ./p2.out 100000_data.csv p2.csv 0.03 10 4 4)
echo "serial: $ts s | P1: $t1 s | P2: $t2 s"
echo "speedup P1 = $(echo "$ts/$t1" | bc -l)"
echo "speedup P2 = $(echo "$ts/$t2" | bc -l)"

../../../.venv/bin/python -c "
import pandas as pd, matplotlib.pyplot as plt
fig, axs = plt.subplots(1, 3, figsize=(18, 6), sharex=True, sharey=True)
for ax, (archivo, nombre) in zip(axs, [('serial.csv', 'Serial'), ('p1.csv', 'Paralelo 1'), ('p2.csv', 'Paralelo 2')]):
    d = pd.read_csv(archivo, header=None, names=['x','y','core'])
    out = d.core == 0
    ax.scatter(d.x[~out], d.y[~out], s=1, c='tab:blue', label='core')
    ax.scatter(d.x[out], d.y[out], s=3, c='tab:red', label='outlier')
    ax.set_title(f'{nombre}: {out.sum()} outliers')
    print(f'{nombre}: {out.sum()} outliers')
axs[0].legend(markerscale=8, loc='upper left')
fig.suptitle('DBSCAN outliers (n=100000, eps=0.03, min_samples=10, 4 hilos, k=4)')
fig.tight_layout(); fig.savefig('outliers.png', dpi=130)
"
CODE=$(command -v code || echo "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code")
"$CODE" -r outliers.png
