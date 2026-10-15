# Proyecto Apertura — DBSCAN paralelo para detección de ruido/outliers con OpenMP

Equipo: Tania Hernandez, Tamara Schnaas
Entrega: **20 de octubre de 2026** (código + documentación, en clase; incluye prueba en vivo con dataset dado por el profesor)

## Qué hace este proyecto

Implementa DBSCAN (algoritmo ingenuo de 2 pasos: detectar puntos *core* de primer orden, luego reclasificar como *core* de segundo orden a los puntos ruido que sean épsilon-alcanzables desde un core) en tres versiones:

1. **Serial** — línea base para medir speedup.
2. **Paralela P1** — considerando toda la matriz de puntos como indivisible.
3. **Paralela P2** — dividiendo la matriz en `k` particiones {2, 4, 8, 16, 32, 64} y uniendo resultados manteniendo consistencia entre fronteras.

## Estructura

```
src/
  serial/dbscan_serial.cpp       — línea base
  parallel/dbscan_p1.cpp         — P1: matriz indivisible, parallel for sobre los puntos
  parallel-2/dbscan_p2.cpp       — P2: cuadrícula de k celdas + halo de ancho eps
notebooks/
  DBSCAN_noise.ipynb             — genera el CSV de entrada y visualiza el CSV de salida
  prueba_dataset.ipynb           — corre las 3 versiones con cualquier CSV y una batería de casos raros
docs/
  code-description.md            — qué hace cada bloque + estrategias de paralelización (P1 y P2)
  experimental-eval.md           — experimento, hardware/software, resultados, IA generativa
experiments/
  generar_datos.py               — genera <n>_data.csv igual que el notebook
  correr_experimento.py          — corre todas las configuraciones (reanudable)
  graficar.py                    — gráficas de speedup
  analizar_celdas.py             — trabajo por celda de P2 (explica su speedup)
  resultados_crudos.csv          — cada corrida
  resultados.csv                 — promedio de 10 repeticiones + speedup
  graficas/                      — PNG de speedup
```

## Cómo correr

```bash
# 1. Compilar (Mac con libomp de Homebrew; Apple clang no acepta -fopenmp directo)
CXX="clang++ -std=c++17 -O2 -Xpreprocessor -fopenmp -I/usr/local/opt/libomp/include -L/usr/local/opt/libomp/lib -lomp"
$CXX src/serial/dbscan_serial.cpp -o dbscan_serial.out
$CXX src/parallel/dbscan_p1.cpp   -o dbscan_p1.out
$CXX src/parallel-2/dbscan_p2.cpp -o dbscan_p2.out

# 2. Ejecutar:  entrada  salida  eps  min_samples  [hilos]  [k]
./dbscan_serial.out 4000_data.csv 4000_results.csv 0.03 10
./dbscan_p1.out     4000_data.csv 4000_results.csv 0.03 10 16
./dbscan_p2.out     4000_data.csv 4000_results.csv 0.03 10 16 64

# 3. Visualizar con DBSCAN_noise.ipynb (celda 2 carga <n_points>_results.csv)

# Experimento completo (desde la raíz del repo; ~1–1.5 h, se puede interrumpir y reanudar)
.venv/bin/python proyectos/dbscan/experiments/correr_experimento.py            # --rapido para probar
.venv/bin/python proyectos/dbscan/experiments/graficar.py
```

## Parámetros del experimento (según el PDF del proyecto)

- **k** (divisiones, solo P2): {2, 4, 8, 16, 32, 64}
- **Número de puntos**: {20000, 40000, 80000, 120000, 140000, 160000, 180000, 200000}
- **Número de hilos**: {1, cores_virtuales/2, cores_virtuales, cores_virtuales*2}
- **10 iteraciones por configuración**, promediadas

## Checklist de entrega (de la rúbrica)

- [x] Código fuente: serial + P1 + P2
- [ ] `docs/code-description.md`: explica cada línea + estrategia de paralelización
- [ ] `docs/experimental-eval.md`: definición del experimento, hardware/software, interpretación de resultados (por qué una versión paralela fue mejor/equivalente a la otra), sección de uso responsable de IA generativa, gráficas, archivo de datos
- [x] Al menos una versión paralela con speedup > 1.5 (si no, penalización de 50%)
- [ ] Ambos integrantes pueden explicar a detalle cualquier parte del código (examen en vivo)