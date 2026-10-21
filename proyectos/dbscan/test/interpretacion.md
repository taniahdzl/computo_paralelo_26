# Interpretación de la prueba: 100,000 puntos, 4 hilos, k = 4

Corrida con `test/correr.sh` sobre `test/100000_data.csv` (`eps = 0.03`, `min_samples = 10`). Las
tres versiones hacen el mismo algoritmo de 2 pasos; solo cambia cómo se reparte el trabajo, así que
el speedup es una comparación justa.

| Versión | Tiempo | Speedup | Outliers |
|---|---|---|---|
| Serial | 7.39 s | 1 | 92 |
| P1, 4 hilos | 1.95 s | **3.79** | 92 |
| P2, 4 hilos, k = 4 | 1.17 s | **6.33** | 92 |

**Validación:** los tres CSV de salida son **idénticos byte por byte** (`cmp`), no solo en el
conteo. Los 92 outliers son los mismos puntos, y en `outliers.png` los tres paneles coinciden. Los
outliers están en las orillas de los dos blobs y en el hueco entre ellos (x ≈ 0.6), donde hay menos
de 10 puntos a distancia 0.03 y ningún core cerca.

---

## P1: speedup 3.79 con 4 hilos

### Idea en una frase
P1 hace las mismas n² distancias que la serial; solo las reparte entre 4 hilos, así que lo más que
puede dar es 4.

| Dato | Valor | Qué significa |
|---|---|---|
| P1 con 1 hilo | 7.26–7.67 s | Igual que la serial: no hay trabajo extra. |
| Speedup con 4 hilos | 3.79 | **Eficiencia de 95%** (3.79 / 4). |

**Por qué sale casi perfecto:**
- 4 hilos caben en los **8 cores físicos** de la Mac, así que cada hilo tiene un core para él solo.
  No hay *hyper-threading* compitiendo por las unidades de punto flotante (que es lo que frenaba a
  P1 con 16 hilos en el experimento grande).
- El Paso 1 tiene iteraciones de igual costo y `schedule(static)` las reparte parejo sin costo de
  reparto. El Paso 2 usa `dynamic, 64` para el desbalance del `continue`/`break`.
- No hay `critical` ni `atomic`: cada hilo escribe solo `es_core[i]` de sus propias `i`.

**Por qué no llega a 4:** crear el equipo de hilos, la barrera implícita entre el Paso 1 y el Paso 2,
y que el Paso 2 (con `dynamic`) se reparte en bloques que tienen que pedirse.

---

## P2: speedup 6.33 con 4 hilos

### Idea en una frase
P2 sí supera el número de hilos porque gana por **dos razones que se multiplican**: hace menos
distancias que la serial (algoritmo) y además reparte las celdas entre hilos (paralelismo).

### Qué pasa con la cuadrícula de 2 × 2
Con k = 4 el plano se parte en cuadrantes por la mitad del rectángulo que contiene a los puntos
(x ≈ 0.67, y ≈ 0.77). Contamos los puntos de cada celda:

| Celda | Puntos propios | Candidatos (con halo) | Distancias (propios × candidatos) | % del trabajo |
|---|---|---|---|---|
| Izquierda, abajo | 22,418 | 29,088 | 0.65 × 10⁹ | 19% |
| Izquierda, arriba | 27,821 | 34,132 | 0.95 × 10⁹ | 28% |
| Derecha, abajo | 12,940 | 18,922 | 0.24 × 10⁹ | 7% |
| Derecha, arriba | 37,209 | 41,920 | **1.56 × 10⁹** | **46%** |
| **Total** | 100,000 | | 3.41 × 10⁹ | |

La serial hace n² = 10 × 10⁹ distancias.

### Descomposición del 6.33

| Factor | Cómo se calcula | Predicho | Medido |
|---|---|---|---|
| Menos trabajo (algoritmo) | 10 × 10⁹ / 3.41 × 10⁹ | 2.93 | 2.85 (serial / P2 con 1 hilo: 7.39 / ~2.6 s) |
| Paralelismo con 4 hilos | 3.41 × 10⁹ / 1.56 × 10⁹ (total / celda más pesada) | 2.19 | 2.22 (~2.6 / 1.17 s) |
| **Total** | 10 × 10⁹ / 1.56 × 10⁹ | **6.4** | **6.33** |

**Conclusión:** el tiempo de P2 lo marca la **celda más cargada**. Con 4 hilos y 4 celdas, cada
hilo toma una celda (`dynamic, 1`), y los otros tres terminan antes y se quedan esperando en la
barrera a la celda de arriba a la derecha, que tiene el 46% del trabajo. Por eso el paralelismo
aporta solo 2.2× y no 4×.

**Por qué el desbalance:** los dos blobs no están centrados en las celdas. El de la derecha es más
denso, y la línea horizontal (y ≈ 0.77) lo parte de forma que la mitad de arriba se queda con casi
3 veces más puntos que la de abajo (37k contra 13k).

---

## P1 contra P2 en esta prueba

| | P1 | P2 |
|---|---|---|
| Distancias | 10 × 10⁹ (n²) | 3.41 × 10⁹ (≈ n²/k + halo) |
| Cuánto aprovecha los 4 hilos | 3.79× (95%) | 2.2× (55%) |
| Speedup total | 3.79 | **6.33** |

P1 **paraleliza mejor**, pero P2 **gana en tiempo** porque hace casi 3 veces menos trabajo. Es lo
mismo que salió en el experimento grande (`docs/experimental-eval.md`).

## Preguntas que nos pueden hacer

1. **¿Cómo es posible un speedup de 6.33 con solo 4 hilos?** No es todo paralelismo: P2 hace 2.9
   veces menos distancias que la serial. El paralelismo aporta otro 2.2×, y 2.9 × 2.2 ≈ 6.4.
2. **¿Por qué P2 no aprovecha los 4 hilos al 100%?** Porque una celda tiene el 46% del trabajo y el
   tiempo lo marca esa celda. Los otros hilos acaban antes y esperan en la barrera.
3. **¿Cómo lo mejorarían?** Con una k más grande que el número de hilos (por ejemplo k = 16 o 32):
   las celdas son más chicas, el trabajo baja más y `dynamic` puede repartirlas mejor. Aquí se usó
   k = 4 porque así lo pide la prueba.
4. **¿Por qué P1 sí llega casi a 4?** Todas sus iteraciones cuestan lo mismo, y 4 hilos caben en los
   8 cores físicos, así que no compiten entre sí.
5. **¿Cómo saben que los resultados son correctos?** Los tres CSV de salida son idénticos byte por
   byte: mismos 92 outliers, mismos puntos.
6. **¿Los tiempos siempre salen iguales?** No; varían un poco entre corridas (P2 dio 1.17–1.37 s). La
   conclusión no cambia.
