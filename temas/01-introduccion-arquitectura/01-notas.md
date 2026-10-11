# 01 — Introducción al cómputo paralelo y arquitecturas paralelas

> Fuente: `Introduccion y arquitecturas paralelas.pdf` (194 diapositivas). Entre paréntesis va la página
> del PDF por si quieres ver la diapositiva original, p. ej. (p93).

## Conceptos clave

### 1. ¿Qué es el cómputo paralelo?
- **Cómputo serial**: el problema se resuelve como una lista de instrucciones que se ejecutan
  **una después de otra** en **un solo CPU** (p43).
- **Cómputo paralelo**: usar **varios recursos de cómputo al mismo tiempo** para resolver un problema
  y así **reducir el tiempo** (p46).
- **Programación paralela**: escribir el programa indicando **explícitamente** qué partes pueden
  correr a la vez en distintos procesadores (p47). OpenMP es eso: tú decides qué se paraleliza.

### 2. Concurrencia vs paralelismo vs serial (p48–p64)
Imagina dos tareas, A y B, cada una con 4 pasos.

| Caso | Qué pasa | ¿Concurrente? | ¿Paralelo? |
|---|---|---|---|
| 1 core, primero todo A y luego todo B | Serial | No | No |
| 1 core, alterna A.1, B.1, A.2, B.2… | Las dos "avanzan" a la vez, pero se turnan | **Sí** | No |
| 2 cores, A en uno y B en otro | Se ejecutan literalmente al mismo tiempo | **Sí** | **Sí** |

**Frase para el examen:** *concurrencia* = varias tareas en progreso en el mismo periodo (pueden
turnarse); *paralelismo* = varias tareas ejecutándose **en el mismo instante** en hardware distinto.
Todo lo paralelo es concurrente, pero no todo lo concurrente es paralelo.

### 3. ¿Qué se necesita para paralelizar? (p65, p69–p70)
1. **Descomponer** el problema en partes.
2. **Resolver las restricciones de orden**: qué partes dependen de cuáles.
3. **Asignar** las partes a los procesadores.
4. **Sincronizar/comunicar**, pero lo menos posible, porque eso frena.
5. Cuidar la **granularidad** (ver abajo).

Ejemplo de la p79: `c[i] = a[i] + b[i]`. Cada `i` es independiente de los demás, así que todas las
iteraciones pueden ir en paralelo. **No hay restricción de orden entre iteraciones.**

### 4. Proceso vs hilo (p74–p78)
- **Proceso**: el "recipiente" con todo lo necesario para correr un programa: su propio espacio de
  memoria (código, datos, pila) y recursos (archivos abiertos, registros, alarmas…).
- **Hilo (thread)**: un flujo de ejecución **dentro** de un proceso.
  - Los hilos de un mismo proceso **comparten** código, datos (memoria global/heap) y archivos.
  - Cada hilo tiene **su propia pila (stack) y sus propios registros**.
- **Consecuencia práctica**: como los hilos comparten memoria, se comunican muy rápido (leen las
  mismas variables), pero también **se pueden pisar** (condiciones de carrera). Las variables locales
  de cada hilo viven en su stack, así que son privadas.
- **Hilos de usuario vs de kernel** (p76–p78): los hilos que crea el programa (usuario) se mapean a
  hilos que el sistema operativo agenda (kernel). Hay tres modelos: **1:1** (cada hilo de usuario
  tiene su hilo de kernel), **N:1** (muchos de usuario sobre uno de kernel, así que no hay paralelismo
  real) y **N:M** (muchos sobre varios).

### 5. Terminología (p80–p86)
| Término | En simple |
|---|---|
| **Núcleo (core)** | Unidad que ejecuta su propio flujo de instrucciones. |
| **Zócalo (socket)** | El "chip" físico donde van los cores; una máquina puede tener varios. |
| **Nodo** | Una computadora completa ("computadora en una caja"). |
| **Tarea** | Un pedazo lógico de trabajo que ejecuta un procesador. |
| **SMP** | Varios procesadores que comparten la misma memoria con el mismo acceso. |
| **Memoria compartida** | Todos los procesadores ven la misma memoria → **OpenMP**. |
| **Memoria distribuida** | Cada máquina ve solo su memoria; para ver la de otra hay que mandar mensajes por red → **MPI**. |
| **Sincronización** | Coordinar tareas; casi siempre implica **esperar**, así que aumenta el tiempo. |
| **Granularidad** | Cuánto cómputo haces entre cada comunicación. **Gruesa** = mucho cómputo y poca comunicación (bueno para paralelizar). **Fina** = poco cómputo y mucha comunicación. |

### 6. ¿Cómo se mide qué tan bien se paraleliza? Speedup (p87–p92)

```
             tiempo con 1 procesador (versión serial)
speedup  =  ----------------------------------------
             tiempo con p procesadores
```

- Speedup 4 = "corre 4 veces más rápido".
- **Speedup ideal (lineal)** = p (con 10 procesadores, 10x). Casi nunca pasa.
- En el ejemplo de clase (p88–p90), con 24 workers el speedup fue de solo ~5x. ¿Es bueno? Depende
  de cuánto del programa era paralelizable: eso lo explica Amdahl.
- **Eficiencia** = speedup / p (qué tanto aprovechas cada procesador). No viene en las diapositivas,
  pero es útil para analizar.

### 7. Ley de Amdahl (p93–p95)

```
                  1
speedup = ---------------
           (1 - f) + f / N
```
- `f` = fracción del programa que **sí** se puede paralelizar.
- `N` = número de procesadores.
- `(1 - f)` = la parte serial. Esa parte **no se acelera nunca**: es el **cuello de botella serial**.
- Si `N → ∞`, `f/N → 0`, así que el **speedup máximo = 1 / (1 - f)**.

| f (parte paralela) | Speedup máximo posible (N infinito) |
|---|---|
| 50% | 2x |
| 75% | 4x |
| 90% | 10x |
| 95% | 20x |

**Ejemplo para hacer a mano:** f = 0.9, N = 16 → `1 / (0.1 + 0.9/16) = 1 / (0.1 + 0.05625) = 1 / 0.15625 = 6.4x`.
Aunque tengas 16 cores, no pasas de 6.4x porque el 10% serial pesa mucho.

**"Amdahl, pero…" (p94):** en la vida real es todavía peor, porque la parte paralela tampoco es
perfecta. Se pierde tiempo por **sincronización**, **desequilibrio de carga** (unos hilos acaban antes
y esperan) y **contención** (todos pelean por la misma memoria o caché).

### 8. ¿Por qué necesitamos paralelismo? (p97–p106)
- **Ley de Moore**: el número de transistores se duplica cada ~2 años. Sigue creciendo, pero ya cerca
  de límites físicos.
- Antes, los programas se aceleraban solos porque **subía la frecuencia del reloj (GHz)**. Eso se
  detuvo hace ~15 años por dos "muros":
  - **Power wall**: más frecuencia → más consumo → **demasiado calor**.
  - **Memory wall**: el CPU se volvió mucho más rápido que la memoria, así que se la pasa **esperando
    datos**.
- Solución de la industria: en vez de un core más rápido, **más cores**. Para aprovecharlos, el
  programador tiene que escribir código paralelo.

### 9. Cómo funciona un procesador (p107–p146), lo esencial
- **Transistores → compuertas lógicas (AND, OR, NOT) → sumadores, registros, memoria → procesador.**
- **Arquitectura Von Neumann**: el programa y los datos viven en la misma memoria; el CPU los trae.
- **Ciclo de ejecución** (p129): traer instrucción (fetch) → avanzar el PC → decodificar → traer datos
  si hace falta → ejecutar → repetir.
- **Reloj**: 3.2 GHz = 3,200 millones de ciclos por segundo.
- **Paralelismo a nivel instrucción (ILP)**, que hace el hardware solo:
  - **Pipeline**: como una línea de ensamblaje; mientras una instrucción se ejecuta, la siguiente ya
    se está decodificando.
  - **Superescalar**: varias unidades que ejecutan instrucciones **independientes** a la vez.
  - Ejemplo (p138–p143): `a = x*x + y*y + z*z`. Las 3 multiplicaciones son independientes y pueden ir
    juntas; las sumas tienen que esperarlas. El **grafo de dependencias** dice qué puede ir en
    paralelo.
  - Límite (p146): los programas no tienen tantas instrucciones independientes, así que agregar más
    unidades deja de ayudar. Por eso se pasó a multi-core.

### 10. Taxonomía de Flynn (p155–p165)
Clasifica las computadoras según cuántos **flujos de instrucciones** y cuántos **flujos de datos** hay.

| | 1 dato | Muchos datos |
|---|---|---|
| **1 instrucción** | **SISD**: computadora serial clásica; determinista. | **SIMD**: todas las unidades hacen la *misma* instrucción sobre datos distintos. GPUs, imágenes, matrices. |
| **Muchas instrucciones** | **MISD**: casi no existe. Ej.: varios filtros sobre la misma imagen; varios algoritmos para descifrar el mismo mensaje. | **MIMD**: cada procesador con su programa y sus datos. **La más común hoy** (tu laptop multi-core). Puede ser no determinista. |

- **SPMD** (Single Program, Multiple Data) es una subcategoría de MIMD: **el mismo programa** en todos
  los hilos, pero cada uno trabaja con datos distintos. **Así funciona OpenMP.**

### 11. Clasificación de MIMD por memoria (p172–p177)
| Tipo | Idea |
|---|---|
| **UMA** (Uniform Memory Access) | Todos los CPUs acceden a la memoria **con el mismo tiempo**. Tu laptop. |
| **NUMA** (Non-Uniform) | Cada grupo de CPUs tiene su memoria "cercana"; acceder a la de otro grupo es **más lento**. Servidores con varios sockets. |
| **COMA** (Cache-Only) | No hay memoria principal; solo cachés con directorio. |

### 12. (No) determinismo y condiciones de carrera (p166–p171)
- **Comunicación síncrona (bloqueante)**: `Enviar()` y `Recibir()` esperan a que el otro lado responda.
- **Asíncrona (no bloqueante)**: `Enviar()` no espera.
- **Condición de carrera** (p170–p171): `i = 0`; el hilo 1 hace `i += 5` y el hilo 2 hace `i += 6`.
  - Si se turnan bien, `i = 11`.
  - Si **los dos leen `i = 0` antes de que el otro escriba**, el hilo 1 escribe 5 y el hilo 2 escribe
    6 encima → `i = 6`. **Se perdió una suma.**
  - El resultado depende de quién llegó primero, así que el programa es **no determinista**.
  - Pasa porque `i += 5` no es una sola operación: es **leer → sumar → escribir**, y otro hilo se puede
    meter en medio.

### 13. Memoria: latencia, ancho de banda y "paros" (p178–p194)
- **Latencia**: cuánto tarda en llegar el **primer** byte que pediste.
- **Ancho de banda**: a qué velocidad llegan los bytes **siguientes** (GB/s).
- **Tiempos de acceso** (CPU Kaby Lake, p184):

  | Dónde está el dato | Ciclos |
  |---|---|
  | Caché L1 | 4 |
  | Caché L2 | 12 |
  | Caché L3 | 38 |
  | RAM (DRAM) | ~248 |

  Ir a RAM es ~60 veces más lento que L1. Por eso importa recorrer la memoria en orden (para que los
  datos ya estén en caché).
- **Paro (stall)**: el CPU se detiene porque espera un dato de memoria.
- Cómo se esconde la latencia:
  - **Prefetching**: el hardware adivina qué vas a pedir y lo trae antes.
  - **Multi-hilo** (p186–p190): mientras un hilo espera memoria, otro usa el CPU. En el ejemplo,
    cada hilo hace 3 ciclos de cómputo y luego espera 12 ciclos por un load; hacen falta
    **5 hilos** para tener el CPU al 100%, porque (3 + 12) / 3 = 5.
- **El cómputo paralelo está limitado por el ancho de banda** (p191): si tu programa casi no calcula y
  solo lee y escribe memoria, agregar hilos ayuda poco, porque todos esperan a la misma memoria.
  Conviene que haya **más operaciones aritméticas que loads/stores**.

### 14. Lenguajes del curso (p29–p32)
| | C++ | Julia | Python |
|---|---|---|---|
| Ejecución | **Compilado** (ahead of time: se traduce a código máquina antes de correr) | **JIT** (se compila mientras corre) | Interpretado |
| Tipado | Estático | Dinámico (soporta estático) | Dinámico |
| Memoria | **Manual** (`new` / `delete`) | Automática | Automática |
| En el curso | OpenMP y MPI | Paralelismo nativo | CUDA (GPUs) |

## Comparaciones / tablas

### Cómo se conecta todo con tu laptop (y con el proyecto)
| Dato | Tu Mac | Por qué importa |
|---|---|---|
| CPU | Intel Core i9-9880H @ 2.3 GHz | — |
| Cores físicos | 8 | Unidades de ejecución reales. |
| Cores virtuales (hilos de hardware) | 16 | **Hyper-Threading**: cada core físico corre 2 hilos para esconder "paros" de memoria (sección 13). |
| Tipo (Flynn) | MIMD | Cada core corre su propio flujo. |
| Memoria | Compartida, UMA, 16 GB | Por eso usamos **OpenMP**. |
| Hilos para el experimento | **{1, 8, 16, 32}** | = {1, virtuales/2, virtuales, virtuales×2}, como pide el PDF del proyecto. |

**Qué esperar con 32 hilos:** solo hay 16 hilos de hardware, así que el SO tiene que turnarlos. Eso
es concurrencia, no más paralelismo. Normalmente no mejora y puede empeorar un poco por el costo de
cambiar entre hilos.

## Código de ejemplo visto en clase

Restricción de orden (p79). Ninguna iteración depende de otra, así que se puede paralelizar:
```cpp
void parallel_task(float *a, float *b, float *c) {
    int n = 7;
    // cada c[i] solo usa a[i] y b[i] -> las 7 iteraciones son independientes
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];
}
```

Contraejemplo, **no** paralelizable tal cual:
```cpp
for (int i = 1; i < n; i++)
    c[i] = c[i - 1] + a[i];   // la iteracion i necesita el resultado de la i-1
```

Ejercicio de la p194 (Julia): una variable compartida `valor -= 1` con hilos, con candados y sin
candados. Sin candado da un resultado incorrecto (carrera); con candado da el correcto, pero es lento
porque los hilos se forman a esperar (overhead de sincronización). La tercera versión **elimina la
variable compartida** (`10000000 - i`): es correcta y rápida. **Moraleja: la mejor sincronización es
la que no necesitas.**

## Preguntas tipo examen
1. Diferencia entre concurrencia y paralelismo, con un ejemplo de algo concurrente pero no paralelo.
2. ¿Qué comparten los hilos de un mismo proceso y qué tiene cada uno propio?
3. Si el 80% de un programa es paralelizable, ¿cuál es el speedup máximo? (→ 1/0.2 = **5x**). ¿Y con
   N = 4? (→ 1/(0.2 + 0.2) = **2.5x**).
4. ¿Por qué ya no se suben los GHz para acelerar los programas? (power wall y memory wall)
5. Clasifica en Flynn: una GPU (SIMD), tu laptop multi-core (MIMD), una calculadora vieja (SISD).
6. UMA vs NUMA.
7. Explica la condición de carrera de `i += 5` / `i += 6` y por qué puede dar 6, 5 u 11.
8. ¿Por qué agregar hilos no siempre acelera? (Amdahl, sincronización, desbalance, ancho de banda de
   memoria, más hilos que cores).
9. Granularidad fina vs gruesa: ¿cuál conviene para paralelizar y por qué?

## Dudas resueltas en conversación con Claude
- **¿Speedup y Amdahl usan el mismo "1 procesador"?** El speedup se mide contra la **versión serial**
  (línea base). En el proyecto, la línea base es `dbscan_serial`, no P1 con 1 hilo, aunque conviene
  reportar las dos.
