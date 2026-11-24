# Números primos con MPI — `ejercicios/mpi_primos.cpp`

> Ejercicio de `MPI.pdf` (p30–p38). Lo que piden: **`MPI_Bcast`**, **3 procesos**, la **estructura
> del `for`** de la p36 (reparto cíclico) y **`MPI_Reduce`**. Teoría en `temas/04-MPI/04-mpi.md`
> (nota 04). El programa también corre la versión serial para calcular el speedup.

## Idea en una frase
Contar cuántos primos hay en `[2, 10]`, `[2, 100]`, …, `[2, 10^N]`. Cada uno de los 3 procesos
revisa **un número de cada 3**, cuenta sus primos y al final se **suman** los tres conteos en el
proceso 0.

## Cómo se compila y se corre
```bash
cd ejercicios
mpic++ -std=c++17 -O2 mpi_primos.cpp -o mpi_primos.out
mpirun -n 3 ./mpi_primos.out 7     # 7 = exponente máximo (10^7). Default 8, máximo 9
```
- `mpic++` es `clang++` con las rutas de `mpi.h` y de la librería ya agregadas (nota 04, §3).
- `-n 3` lanza **3 procesos**. No son hilos: cada uno es una copia del programa **con su propia
  memoria** (nota 04, §2).

Salida (encabezado + una línea por rango):
```
N  primos_paralelo  primos_serial  t_serial  t_paralelo  speedup
```

## Recorrido del código, bloque por bloque

### 1. Prueba de primalidad — `ejercicios/mpi_primos.cpp:18-23`
```cpp
bool esPrimo(int possible_prime) {
    int limite = (int)trunc(sqrt((double)possible_prime));
    for (int divisor = 2; divisor <= limite; divisor++)
        if (possible_prime % divisor == 0) return false;
    return true;
}
```
- Busca un divisor de 2 a `√n` (p35). Si encuentra uno, **no es primo**.
- **Por qué basta con llegar a `√n`:** si `n = a·b` con `a ≤ b`, entonces `a ≤ √n`. Si hubiera un
  divisor, habría uno antes de la raíz.
- **El costo depende del número:** un par se descarta en la primera división (`% 2`), pero un primo
  grande hace ~`√n` divisiones. Esto es lo que causa el desbalance (ver "Costo y rendimiento").

### 2. Versión serial — `ejercicios/mpi_primos.cpp:27-32`
`contarPrimosSerial(limite)` es el mismo ciclo de `ejercicios/primos_serial.cpp`: **un solo
proceso** revisa del 2 al `limite`. Sirve de **línea base** para el speedup (nota 01, §6: el speedup
se mide contra la versión serial). Solo la llama el proceso 0.

### 3. Arranque de MPI — `ejercicios/mpi_primos.cpp:35-41`
```cpp
MPI_Init(&argc, &argv);
MPI_Comm_rank(MPI_COMM_WORLD, &id);              // quién soy: 0, 1 o 2
MPI_Comm_size(MPI_COMM_WORLD, &num_processes);   // cuántos somos: 3
```
- Estructura de la p16–p17. **Desde aquí, los 3 procesos ejecutan las mismas líneas.** Lo único que
  los distingue es `id` (modelo SPMD, nota 04 §2).
- `max_exponente` se lee de `argv` en **los 3 procesos**: todos reciben los mismos argumentos de
  `mpirun`, así que no hace falta mandarlo.

### 4. Ciclo de rangos y cronómetro — `ejercicios/mpi_primos.cpp:45-51`
```cpp
if (id == 0) {
    limite *= 10;
    start = MPI_Wtime();
}
```
- **Solo el proceso 0** calcula el nuevo límite (10, 100, 1000, …) y prende el cronómetro (p32).
- En los procesos 1 y 2, `limite` se queda viejo. Por eso hace falta el `Bcast`.

### 5. `MPI_Bcast`: el 0 avisa el límite — `ejercicios/mpi_primos.cpp:54`
```cpp
MPI_Bcast(&limite, 1, MPI_INT, 0, MPI_COMM_WORLD);
```
| Argumento | Valor | Significa |
|---|---|---|
| `&limite` | dirección | En el 0 es **lo que se manda**; en el 1 y el 2 es **dónde se recibe**. |
| `1` | count | Un solo elemento. |
| `MPI_INT` | tipo | Es un `int` (p21). |
| `0` | root | El que manda es el proceso 0. |
| `MPI_COMM_WORLD` | comunicador | A todos los procesos. |

- Es **colectiva**: los 3 procesos llaman a la misma función. No hay un `Send` de un lado y un `Recv`
  del otro (nota 04, §8).
- Al regresar, **los 3 tienen el mismo `limite`**.

### 6. El `for` con reparto cíclico — `ejercicios/mpi_primos.cpp:57-59`
```cpp
int primos_locales = 0;
for (int possible_prime = 2 + id; possible_prime <= limite; possible_prime += num_processes)
    if (esPrimo(possible_prime)) primos_locales++;
```
Es la estructura de la p36. Cada proceso **empieza en `2 + id`** y **salta de `num_processes` en
`num_processes`**. Con 3 procesos:

| Proceso | Empieza en | Revisa |
|---|---|---|
| 0 | 2 | 2, 5, 8, 11, 14, … |
| 1 | 3 | 3, 6, 9, 12, 15, … |
| 2 | 4 | 4, 7, 10, 13, 16, … |

- **Entre los tres cubren todos los números del 2 al `limite`, sin repetir ninguno**: cada número
  `x` lo revisa exactamente el proceso `(x − 2) % 3`.
- No hay que calcular dónde empieza y termina un bloque, y funciona igual si `limite` no es múltiplo
  de 3.
- `primos_locales` es **de cada proceso** (cada uno tiene su copia en su memoria). Nadie más la toca.

### 7. `MPI_Reduce`: juntar los conteos — `ejercicios/mpi_primos.cpp:62-63`
```cpp
int primos_totales = 0;
MPI_Reduce(&primos_locales, &primos_totales, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
```
| Argumento | Valor | Significa |
|---|---|---|
| `&primos_locales` | envío | Lo que aporta cada proceso. |
| `&primos_totales` | recibo | Dónde queda el resultado. **Solo es válido en el root.** |
| `1`, `MPI_INT` | count, tipo | Un `int` por proceso. |
| `MPI_SUM` | operación | Sumar los aportes (p29). |
| `0` | root | El resultado le llega al proceso 0. |

- Es el equivalente de `reduction(+:suma)` de OpenMP (nota 02): cada uno acumula en lo suyo y al
  final se combina **una sola vez**.
- También es colectiva: si un proceso no la llamara, los otros se quedarían esperando.

### 8. Tiempo, serial e impresión (solo el 0) — `ejercicios/mpi_primos.cpp:65-77`
- `end = MPI_Wtime()` va **después** del `Reduce`. El `Reduce` no termina hasta que llegan los 3
  conteos, así que `t_paralelo` incluye el trabajo del proceso más lento (p32, nota 04 §9).
- La versión serial se mide **después** de detener el cronómetro paralelo, con su propio
  `MPI_Wtime()`. Así sus tiempos no se mezclan.
- Mientras el 0 corre la serial, el 1 y el 2 ya están en el `MPI_Bcast` del siguiente rango,
  esperando el nuevo `limite`. No estorban la medición paralela, porque esta empieza hasta que el 0
  vuelve a prender el cronómetro.
- `speedup = t_serial / t_paralelo` (nota 01, §6).

### 9. Cierre — `ejercicios/mpi_primos.cpp:80`
`MPI_Finalize()` cierra el ambiente MPI. Después de esta línea no se puede llamar a ninguna `MPI_…`.

## Decisiones de paralelización y su justificación
| Decisión | Alternativa descartada | Por qué | Fuente |
|---|---|---|---|
| `MPI_Bcast` del `limite` | Que cada proceso lo calcule solo | Es lo que pide el ejercicio y es el patrón de la p34: un dato que solo tiene el 0 se manda a todos | p34, nota 04 §8 |
| Reparto **cíclico** (`2+id`, salto `P`) | Bloques contiguos (`[2, L/3]`, …) | Con bloques, al último proceso le tocan los números más grandes, que son los más caros. El cíclico mezcla chicos y grandes | p36 |
| `MPI_Reduce` con `MPI_SUM` | `Gather` de los 3 conteos y sumar a mano en el 0 | Solo se necesita el total, no cada conteo | p29, nota 04 §8 |
| Root = 0 en `Bcast` y `Reduce` | Otro proceso | Convención de clase: el 0 coordina, mide e imprime | p32 |
| Cronómetro solo en el 0, del `Bcast` al `Reduce` | Medir en cada proceso | El `Reduce` ya espera a todos, así que un solo reloj mide el tiempo total | p32 |
| Serial dentro del mismo programa | Correr `primos_serial.out` aparte | Mismo `esPrimo`, mismo compilador, mismo momento: la comparación es justa. *No viene en las diapositivas* | decisión nuestra |

## Por qué es correcto
- **No hay memoria compartida**: cada proceso tiene sus propias `limite`, `primos_locales` y
  `primos_totales`. Por eso **no puede haber condiciones de carrera** (nota 04, §2). Todo lo que se
  comparte pasa por `Bcast` y `Reduce`.
- **Cada número se revisa exactamente una vez**: el número `x` solo le toca al proceso
  `(x − 2) % 3`. No falta ni sobra ninguno.
- **No hay deadlock**: los 3 procesos llaman a las mismas colectivas (`Bcast`, `Reduce`), en el
  mismo orden y con el mismo root, en cada vuelta del ciclo.
- **Validado**: en cada rango, `primos_paralelo == primos_serial`, y además coinciden con la tabla de
  la p30: 4, 25, 168, 1229, 9592, 78498, 664579 y 5761455 (hasta 10^8).

## Costo y rendimiento

Medido en esta Mac (i9, 8 cores físicos) con `mpirun -n 3 ./mpi_primos.out 8`. **Una sola corrida,
con la máquina cargada** (load average ≈ 19), así que los tiempos absolutos salieron casi el doble
que los de la nota 04. El speedup sí es comparable, porque el serial y el paralelo corrieron bajo la
misma carga.

| Rango | Primos | t_serial (s) | t_paralelo (s) | Speedup |
|---|---|---|---|---|
| [2, 10] | 4 | 0.000000 | 0.000252 | 0 |
| [2, 10^3] | 168 | 0.000033 | 0.000019 | 1.7 |
| [2, 10^5] | 9592 | 0.0111 | 0.0069 | 1.6 |
| [2, 10^6] | 78498 | 0.265 | 0.144 | 1.8 |
| [2, 10^7] | 664579 | 7.08 | 3.81 | 1.9 |
| [2, 10^8] | 5761455 | 183.0 | 102.8 | **1.8** |

- **Rangos chicos:** el speedup es ~0. Arrancar `Bcast` y `Reduce` cuesta más que revisar 10
  números (granularidad fina, nota 01).
- **Rangos grandes:** con 3 procesos, el speedup se queda en **~1.8x**, no en 3x. El motivo es el
  desbalance de abajo.

### Por qué no llega a 3x: el proceso 1 casi no trabaja
Con P = 3, el proceso 1 revisa `3, 6, 9, 12, …`: **solo múltiplos de 3**. Todos menos el 3 se
descartan en la primera o la segunda división (`% 2` o `% 3`). Los primos, que son los caros, caen
todos en el proceso 0 y el 2.

Lo medimos contando cuántas divisiones (`%`) hace cada proceso hasta 10^7:

| Proceso | % del trabajo total |
|---|---|
| 0 | 49.9% |
| 1 | **0.3%** |
| 2 | 49.9% |

- El tiempo paralelo lo marca el proceso más cargado (≈ 50% del trabajo), así que **el speedup máximo
  posible es ≈ 2.0**. Medimos 1.8; la diferencia es la comunicación y el ruido de la máquina cargada.
- Es un problema de **balance de carga** (como `static` vs `dynamic` en la nota 02): el reparto es
  parejo en **cantidad de números**, pero no en **costo**.
- Pasa lo mismo con cualquier P par (nota 04, "Hallazgo"): los procesos que solo reciben pares no
  trabajan, y el speedup queda en P/2.
- `ejercicios/mpi_primos_balanceado.cpp` (que solo reparte impares) **no lo arregla con P = 3**: ahí
  el proceso 0 recibe `3, 9, 15, …`, que también son múltiplos de 3. ⚠️ Pendiente de medir, si
  quieren incluirlo en el reporte.

## Preguntas que nos pueden hacer en la defensa
1. **¿Por qué hace falta el `MPI_Bcast` si `limite` es una variable del programa?** — Porque cada
   proceso tiene su propia memoria. Solo el 0 hace `limite *= 10`; sin el `Bcast`, el 1 y el 2 se
   quedan con el valor viejo.
2. **¿Qué proceso revisa el número 100?** — `(100 − 2) % 3 = 2`, así que el proceso 2 (empieza en 4
   y va de 3 en 3: 4, 7, …, 97, 100).
3. **¿Qué tiene `primos_totales` en los procesos 1 y 2 después del `Reduce`?** — Sigue en 0. El
   resultado del `Reduce` solo llega al root.
4. **¿Por qué `end = MPI_Wtime()` va después del `Reduce` y no antes?** — El `Reduce` espera los 3
   conteos. Si se midiera antes, solo contaría lo que tardó el proceso 0.
5. **¿Puede haber condiciones de carrera en `primos_locales`?** — No. En MPI no hay variables
   compartidas: cada proceso tiene su propia copia.
6. **¿Por qué el speedup con 3 procesos es ~1.8 y no 3?** — El proceso 1 solo recibe múltiplos de 3,
   que se descartan en 1–2 divisiones. Hace el 0.3% del trabajo, así que en la práctica trabajan 2
   procesos.
7. **¿Por qué en `[2, 10]` el speedup es 0?** — Hay tan poco trabajo que la comunicación (`Bcast` +
   `Reduce`) tarda más que el cálculo.
8. **¿Por qué reparto cíclico y no bloques?** — Los números grandes cuestan más (más divisores que
   probar). Con bloques, al último proceso le tocarían todos los caros.
9. **¿Qué diferencia hay entre `Reduce` y `Gather` aquí?** — `Gather` le daría al 0 los 3 conteos por
   separado; `Reduce` le da directamente la suma.
10. **¿Qué pasa si un proceso no llama al `MPI_Bcast`?** — Los demás se quedan esperándolo para
    siempre: las colectivas las tienen que llamar todos.
