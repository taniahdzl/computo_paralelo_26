# Proyecto Apertura — DBSCAN paralelo para detección de ruido/outliers con OpenMP

Equipo: <nombre 1>, <nombre 2>
Entrega: **20 de octubre de 2026** (código + documentación, en clase; incluye prueba en vivo con dataset dado por el profesor)

## Qué hace este proyecto

Implementa DBSCAN (algoritmo ingenuo de 2 pasos: detectar puntos *core* de primer orden, luego reclasificar como *core* de segundo orden a los puntos ruido que sean épsilon-alcanzables desde un core) en tres versiones:

1. **Serial** — línea base para medir speedup.
2. **Paralela P1** — considerando toda la matriz de puntos como indivisible.
3. **Paralela P2** — dividiendo la matriz en `k` particiones {2, 4, 8, 16, 32, 64} y uniendo resultados manteniendo consistencia entre fronteras.

## Estructura

```
src/
  serial/dbscan_serial.cpp
  paralelo_p1/dbscan_p1.cpp
  paralelo_p2/dbscan_p2.cpp
notebooks/
  DBSCAN_noise.ipynb      — genera el CSV de entrada y visualiza el CSV de salida
docs/
  descripcion-codigo.md          — qué hace cada línea + estrategia de paralelización (P1 y P2)
  evaluacion-experimental.md     — definición del experimento, hardware/software, resultados, IA generativa
experimentos/
  resultados.csv           — promedio de 10 iteraciones por configuración
  graficas/speedup.png
```

## Cómo correr

```bash
# 1. Generar el dataset de entrada (desde el notebook DBSCAN_noise.ipynb)
#    produce <n_points>_data.csv

# 2. Compilar
g++ -fopenmp src/serial/dbscan_serial.cpp -o dbscan_serial
g++ -fopenmp src/paralelo_p1/dbscan_p1.cpp -o dbscan_p1
g++ -fopenmp src/paralelo_p2/dbscan_p2.cpp -o dbscan_p2

# 3. Ejecutar (ejemplo)
./dbscan_serial <n_points>_data.csv <n_points>_results.csv
./dbscan_p1 <n_points>_data.csv <n_points>_results.csv <num_hilos>
./dbscan_p2 <n_points>_data.csv <n_points>_results.csv <num_hilos> <k>

# 4. Visualizar resultados con DBSCAN_noise.ipynb (celda 2, carga <n_points>_results.csv)
```

## Parámetros del experimento (según el PDF del proyecto)

- **k** (divisiones, solo P2): {2, 4, 8, 16, 32, 64}
- **Número de puntos**: {20000, 40000, 80000, 120000, 140000, 160000, 180000, 200000}
- **Número de hilos**: {1, cores_virtuales/2, cores_virtuales, cores_virtuales*2}
- **10 iteraciones por configuración**, promediadas

## Checklist de entrega (de la rúbrica)

- [ ] Código fuente: serial + P1 + P2
- [ ] `docs/descripcion-codigo.md`: explica cada línea + estrategia de paralelización
- [ ] `docs/evaluacion-experimental.md`: definición del experimento, hardware/software, interpretación de resultados (por qué una versión paralela fue mejor/equivalente a la otra), sección de uso responsable de IA generativa, gráficas, archivo de datos
- [ ] Al menos una versión paralela con speedup > 1.5 (si no, penalización de 50%)
- [ ] Ambos integrantes pueden explicar a detalle cualquier parte del código (examen en vivo)