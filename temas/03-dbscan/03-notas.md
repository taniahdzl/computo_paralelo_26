# 03 — DBSCAN para detectar ruido (Proyecto Apertura)

> Fuente: `ProyectoApertura2026bDBSCAN.pdf` (15 diapositivas). Entre paréntesis va la página, p. ej. (p5).
> Esta nota explica el **algoritmo** y las **ideas** de paralelización. El detalle línea por línea del
> código va en `proyectos/dbscan/docs/code-description.md`.

## Conceptos clave

### 1. ¿Qué es DBSCAN? (p2–p3)
**D**ensity-**B**ased **S**patial **C**lustering of **A**pplications with **N**oise: agrupa puntos
según la **densidad**. Donde hay muchos puntos juntos hay un grupo (cluster); los puntos aislados son
**ruido u outliers**. En este proyecto **solo** nos importa separar **ruido** de **no ruido**, no qué
cluster es cuál.

### 2. Parámetros (p4)
| Parámetro | Significa | Valor en el notebook |
|---|---|---|
| `eps` (épsilon) | Radio del "vecindario" de un punto | `0.03` |
| `min_samples` | Cuántos puntos, **contándose a sí mismo**, debe haber a distancia ≤ eps para que la zona sea densa | `10` |

### 3. Tres tipos de punto (p4, p7, p10)
| Tipo | Regla | Etiqueta en el CSV de salida |
|---|---|---|
| **Core de primer orden** | Tiene ≥ `min_samples` puntos a distancia ≤ eps. Está "en medio" de una zona densa. | 1 |
| **Core de segundo orden** | **No** tiene suficientes vecinos, pero está a distancia ≤ eps de **algún core de primer orden**. Está en la orilla de un grupo. (En el DBSCAN clásico se le llama *border point*.) | 1 |
| **Ruido / outlier** | Ninguna de las dos anteriores. | 0 |

### 4. Distancia euclidiana (p5)
```
d(P1, P2) = sqrt( (x2 - x1)^2 + (y2 - y1)^2 )
```

### 5. Algoritmo ingenuo de 2 pasos (p5–p10)
**Paso 1.** Para **cada** punto `i`, cuenta cuántos puntos `j` (incluido él mismo, porque d(i,i) = 0)
están a distancia ≤ eps. Si son ≥ `min_samples`, `i` es **core de primer orden**; si no,
**ruido (por ahora)**.

**Paso 2.** Para **cada** punto que quedó como ruido, busca si hay **algún core de primer orden** a
distancia ≤ eps. Si lo hay, se **reetiqueta** como **core de segundo orden**. Si no, se queda como ruido.

Ejemplo de las diapositivas (min_samples = 5): el punto verde dentro del círculo no tiene 5 vecinos,
así que en el Paso 1 es ruido. Pero está dentro del radio eps de un punto amarillo (core), así que en
el Paso 2 pasa a core de segundo orden. Los dos verdes de afuera siguen siendo ruido.

**Detalle clave del Paso 2:** solo se vale "colgarse" de un core de **primer** orden. Un punto que
acaba de volverse core de segundo orden **no** puede rescatar a otro. Por eso la versión serial lee
de una **copia fija** del resultado del Paso 1 (`es_core_paso1`) y escribe en `es_core`. Si leyera y
escribiera el mismo arreglo, el resultado dependería del orden del `for` (y en paralelo, de qué hilo
llegó primero).

### 6. Complejidad: ¿por qué tarda tanto?
- Paso 1: por cada uno de los `n` puntos se revisan los `n` puntos, así que son **n² distancias**.
- Paso 2: en el peor caso, otras n². En la práctica menos, porque se salta los core y hace `break` al
  encontrar el primer core cercano.
- Con n = 200,000 son **40,000 millones** de distancias **solo en el Paso 1**. Duplicar n **cuadruplica**
  el tiempo.
- Lo bueno: es **muchísimo cómputo por cada dato leído**. Los arreglos `x` y `y` de 200,000 doubles
  pesan ~3.2 MB, así que casi caben en caché. Es lo contrario de la suma de vectores (nota 02): es
  **granularidad gruesa** y **debería escalar bien** con los hilos.

### 7. Entrada y salida (p14, notebook `DBSCAN_noise.ipynb`)
- **Celda 1 del notebook**: genera `n_points` puntos con `make_blobs` (4 grupos), los guarda en
  `<n>_data.csv` (`x,y` sin encabezado, 3 decimales) y dibuja lo que encuentra el DBSCAN de sklearn.
- **Tu programa**: lee `<n>_data.csv` y escribe `<n>_results.csv` con `x,y,label` (1 = core, 0 = ruido).
- **Celda 2 del notebook**: lee `<n>_results.csv` y dibuja el ruido de otro color.
- **Validación hecha:** con 4000 puntos, tu `dbscan_serial` encontró **59 puntos de ruido, igual que
  sklearn, y con 0 etiquetas distintas** punto por punto. Tu definición coincide con la de sklearn:
  `min_samples` incluye al propio punto y el core de 2º orden equivale a su *border point*.

## Lo que pide el proyecto (p11–p15)

### Tres versiones
| Versión | Idea |
|---|---|
| **Serial** ✅ | Línea base para el speedup. Ya está en `src/serial/dbscan_serial.cpp`. |
| **P1** | Paralelizar considerando **toda la matriz de puntos como indivisible**. |
| **P2** | **Dividir la matriz en `k` partes** (k = 2, 4, …, 64) y resolver cada parte en paralelo. Pregunta del profesor: **¿cómo unir los resultados para mantener consistencia?** |

### Ideas para P1, a discutir al implementarla
- Los dos pasos son un `for` sobre `i` donde cada iteración es **independiente**: el punto `i` solo
  **escribe** en `es_core[i]` y solo **lee** `x`, `y` y `es_core_paso1`, que nadie modifica. Así que
  `#pragma omp parallel for` sobre el `for` de afuera **no tiene carreras** y no necesita `critical`.
- `vecinos` se declara **dentro** del for, así que es privada por hilo.
- Entre el Paso 1 y el Paso 2 hay una **restricción de orden**: el Paso 2 necesita **todo** el
  resultado del Paso 1. La barrera implícita al final del primer `parallel for` lo garantiza.
- **¿`static` o `dynamic`?** En el Paso 1 todas las iteraciones cuestan lo mismo (siempre recorren n),
  así que `static` debería bastar. En el Paso 2 **no**: los core se saltan (`continue`) y el `break`
  corta antes o después, así que hay desbalance y vale la pena probar `dynamic`. Es justo el tema de
  `schedule` de la nota 02.

### Ideas para P2, el reto de la consistencia
- **Dividir por índice** (los primeros n/k puntos, los siguientes…) no es la idea del dibujo de la p11.
  Ahí se divide el **plano** (k = 2 → 2 mitades; k = 4 → 4 cuadrantes…). Para k = 8, 16, 32, 64 se
  puede usar una cuadrícula: 2×4, 4×4, 4×8, 8×8.
- **El problema de las fronteras:** un punto pegado al borde de su cuadro tiene vecinos **del otro
  lado**. Si cada partición solo mira sus propios puntos, ese punto "pierde vecinos" y se marca como
  **ruido por error**.
- **Solución típica: "halo" o zona fantasma.** Cada partición considera también los puntos de las
  particiones vecinas que están a ≤ eps de su borde. Solo los **lee** para contar vecinos; solo
  **etiqueta** a sus propios puntos. Como cada punto pertenece a exactamente una partición, unir los
  resultados es juntar las etiquetas, sin conflictos.
- En el Paso 2, un punto puede necesitar un core de la partición de al lado. Por eso el Paso 1 debe
  terminar **en todas las particiones** antes de empezar el Paso 2 (barrera).
- **Por qué P2 podría ganarle a P1:** cada punto se compara solo con los puntos de su cuadro y su halo,
  no con los n. Hace **menos distancias en total**, así que la ventaja es **del algoritmo**, no solo del
  paralelismo. Con k grande, el halo crece en proporción y además hay **desbalance**: los blobs no están
  repartidos parejo, así que unos cuadros tienen muchos puntos y otros casi ninguno. Eso sirve para
  responder "¿por qué una versión fue mejor que la otra?".
- **Validación obligatoria:** P1 y P2 deben dar **exactamente las mismas etiquetas** que el serial.

### Experimento (p14)
| Parámetro | Valores |
|---|---|
| k (solo P2) | {2, 4, 8, 16, 32, 64} |
| Número de puntos | {20000, 40000, 80000, 120000, 140000, 160000, 180000, 200000} |
| Hilos | {1, virtuales/2, virtuales, virtuales×2} = **{1, 8, 16, 32}** en tu Mac |
| Repeticiones | 10 por configuración, **promediadas** |

- Medir con `omp_get_wtime()` **solo los dos pasos** (sin leer ni escribir archivos), igual en las tres
  versiones.
- `speedup = tiempo_serial / tiempo_paralelo`, para el mismo número de puntos.
- **Gráfica de speedups**: P1 y P2 contra la serial.
- **Tiempo:** son 8×10 = 80 corridas del serial, 8×4×10 = 320 de P1 y 8×4×6×10 = 1,920 de P2. Con
  n = 200,000 cada corrida puede tardar bastante, así que **hay que automatizarlo con un script y
  dejarlo corriendo** (de noche, con la laptop conectada).

### Entregables y calificación (p13, p15)
- Código: serial + P1 + P2.
- Escrito 1: descripción del código y de las estrategias de paralelización.
- Escrito 2: evaluación experimental. Incluye definición del experimento, hardware/software,
  interpretación (**por qué una versión paralela fue mejor o equivalente a la otra**), sección de **uso
  responsable de IA generativa** (lineamientos IEEE y Elsevier), gráficas y archivo con los datos.
- Peso: 2 puntos de la calificación final (1.2 ejecución + 0.8 documentos).
- ⚠️ **Al menos una versión paralela con speedup > 1.5**, o pierdes 50%.
- ⚠️ **Si no puedes explicar a detalle cualquier parte del programa, pierdes 80%.**
- ⚠️ Entrega: **martes 20 de octubre de 2026, en clase.** Cada día de retraso es −20%, y entregar
  después de la hora cuenta como un día.

## Comparaciones / tablas

### DBSCAN vs suma de vectores (por qué uno escala mejor)
| | Suma de vectores | DBSCAN ingenuo |
|---|---|---|
| Trabajo por dato | 1 suma | n distancias |
| Complejidad | O(n) | O(n²) |
| Limitado por | Ancho de banda de memoria | Cómputo (CPU) |
| Speedup esperado | Bajo, se estanca (~4x medido) | Alto, cercano al número de cores físicos |

## Código de ejemplo visto en clase
No hubo código en las diapositivas del proyecto. La implementación serial comentada está en
`proyectos/dbscan/src/serial/dbscan_serial.cpp`.

Para correrla (después de generar `4000_data.csv` con el notebook):
```bash
cd proyectos/dbscan/src/serial
clang++ -std=c++17 -O2 -Xpreprocessor -fopenmp -I/usr/local/opt/libomp/include \
    -L/usr/local/opt/libomp/lib -lomp dbscan_serial.cpp -o dbscan_serial.out
./dbscan_serial.out 4000_data.csv 4000_results.csv 0.03 10
```

## Preguntas tipo examen / defensa del proyecto
1. Define core de 1er orden, core de 2º orden y ruido. ¿Por qué un punto de 2º orden no puede
   "rescatar" a otro?
2. ¿Por qué el Paso 2 lee de `es_core_paso1` y no de `es_core`? ¿Qué pasaría en paralelo si no?
3. ¿Cuál es la complejidad y qué pasa con el tiempo si duplico los puntos?
4. En P1, ¿por qué no hace falta `critical` al escribir `es_core[i]`?
5. ¿Qué `schedule` usaste en cada paso y por qué?
6. En P2, ¿cómo evitas marcar como ruido un punto que está en la frontera entre dos particiones?
7. ¿Por qué con 32 hilos no mejora (o empeora) respecto a 16 en tu máquina?
8. ¿Por qué P1 fue mejor, peor o equivalente a P2?
9. ¿Por qué no se mide el tiempo de leer el CSV?

## Dudas resueltas en conversación con Claude
- **¿Mi serial da lo mismo que sklearn?** Sí: con 4000 puntos, `eps = 0.03` y `min_samples = 10`, hay
  0 diferencias (59 puntos de ruido en ambos).
- **¿Puedo usar `distancia² <= eps²` para no calcular la raíz?** Es matemáticamente equivalente y
  ahorra trabajo. Si se cambia, hay que hacerlo **igual en las tres versiones** para que el speedup sea
  justo, y volver a validar contra sklearn. (Pendiente de decidir.)
