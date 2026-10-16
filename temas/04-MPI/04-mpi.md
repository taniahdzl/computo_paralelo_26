# 04 — MPI (Message Passing Interface)

> Fuente: `MPI.pdf` (38 diapositivas). Entre paréntesis va la página del PDF, p. ej. (p17).
> Los ejercicios están en `ejercicios/mpi_*.cpp` y `ejercicios/primos_serial.cpp`; todos se
> compilaron y corrieron en tu Mac, y las salidas de esta nota son reales.

## Conceptos clave

### 1. ¿Qué es MPI? (p2–p5)
- Es un **estándar de paso de mensajes**: una librería de funciones (`MPI_Send`, `MPI_Recv`,
  `MPI_Bcast`…), no un lenguaje ni directivas como OpenMP. Open MPI y Microsoft MPI son
  **implementaciones** del estándar.
- Se diseñó para **memoria distribuida** (p3): cada CPU tiene **su propia memoria** y se comunican
  por una red. Hoy corre casi en cualquier arquitectura: distribuida, compartida o **híbrida**
  (varios nodos, cada uno multicore).
- **Paso de mensajes** (p4): el proceso 0 y el proceso 1 tienen **cada uno su propia variable `Y`**.
  Para que el proceso 1 conozca el `Y` del 0, el 0 hace `MPI_Send` y el 1 hace `MPI_Recv`.
  **No hay variables compartidas**: todo lo que se comparte, se manda explícitamente.
- ¿Por qué usarlo? (p5): **rendimiento**, **estandarización/portabilidad** (el mismo código corre en
  tu laptop y en un clúster) y **funcionalidad** (muchas rutinas ya hechas).

### 2. MPI vs OpenMP (lo más preguntable)
| | OpenMP (nota 02) | MPI |
|---|---|---|
| Unidad de ejecución | **Hilos** dentro de un proceso | **Procesos** independientes |
| Memoria | **Compartida**: todos ven el mismo arreglo | **Distribuida**: cada proceso tiene la suya |
| Cómo se comparte un dato | Leyéndolo (y cuidando las carreras) | **Mandando un mensaje** |
| Riesgo típico | Condición de carrera | **Deadlock** (dos procesos esperándose) y costo de comunicación |
| Escala hasta | Los cores de **una** máquina | **Muchas** máquinas (un clúster) |
| Cómo se escribe | `#pragma omp …` sobre código serial | Llamadas explícitas a funciones `MPI_…` |
| Cuántos | `OMP_NUM_THREADS=8 ./prog` | `mpirun -n 8 ./prog` |

- Las dos son **SPMD** (*single program, multiple data*): **todos los procesos corren el mismo
  programa**, y cada uno decide qué hacer según su `rank` (`if (rank == 0) …`).
- En MPI **no hay fork-join**: los N procesos existen desde que arranca `mpirun` hasta que termina el
  programa.

### 3. Instalación y compilación en tu Mac (p6–p12)
- Diapositivas: `brew install openmpi` (p6). En Homebrew la fórmula se llama `open-mpi`
  (`openmpi` es un alias). En tu Mac (Intel, macOS 26) **no había binario precompilado**: Homebrew
  compiló GCC desde el código fuente (es dependencia por `gfortran`) y tardó **~1 h 15 min** (GCC ~75 min + Open MPI 8 min; quedó Open MPI 5.0.11). Ya no hay que repetirlo.
- **Compilar:** `mpic++` es un "envoltorio" de `clang++` que ya agrega el `-I` de `mpi.h` y el `-L`/`-l`
  de la librería (lo que en Windows se hace a mano con `-I ${MSMPI_INC} -L ${MSMPI_LIB64} -lmsmpi`, p11):
  ```bash
  mpic++ -std=c++17 -O2 mpi_hello_world.cpp -o mpi_hello_world.out
  ```
- **Correr:** `mpirun` (en Windows, `mpiexec`; en Open MPI son el mismo programa) (p12):
  ```bash
  mpirun -n 4 ./mpi_hello_world.out
  ```
  Si pides más procesos que cores físicos (8 en tu Mac), Open MPI se niega (*"There are not enough
  slots available"*, lo comprobé con `-n 9`). Agrega
  `--oversubscribe`: `mpirun --oversubscribe -n 32 ./mpi_primos.out`.
- **VS Code** (p8–p11): el error `cannot open source file "mpi.h"` es solo de IntelliSense. Se
  arregla agregando la carpeta `include` de Open MPI al `includePath`. En la diapositiva viene
  `/opt/homebrew/...` (Mac con Apple Silicon); en tu Mac Intel es `/usr/local/include`.

### 4. Comunicadores, size y rank (p13–p14)
- **Comunicador** (`MPI_Comm`): un grupo de procesos que pueden hablar entre sí. **`MPI_COMM_WORLD`**
  = todos los procesos que lanzó `mpirun`.
- **Rank**: el id de un proceso **dentro de un comunicador**, de 0 a size−1. La p13 muestra que el
  mismo proceso puede tener rank 5 en `MPI_COMM_WORLD` y rank 1 en un subgrupo (comunicador más chico).
- `MPI_Comm_rank(comm, &rank)` → quién soy. `MPI_Comm_size(comm, &size)` → cuántos somos.
- **Tipos de comunicación** (p14): **punto a punto** (un proceso con otro: `Send`/`Recv`) y
  **colectiva** (todos los del comunicador a la vez: `Bcast`, `Scatter`, `Gather`, `Reduce`).

### 5. Estructura de un programa MPI (p16–p17)
```cpp
#include <mpi.h>                                   // 1. incluir MPI
int main(int argc, char** argv) {
    // codigo serial (lo corren TODOS los procesos, cada uno por su cuenta)
    MPI_Init(&argc, &argv);                        // 2. arranca el ambiente MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    // 3. trabajo + llamadas de paso de mensajes
    MPI_Finalize();                                // 4. termina el ambiente MPI
    // codigo serial
}
```
- **Ojo:** el "código serial" de antes y después **también lo ejecutan todos los procesos**. No es como
  OpenMP, donde fuera de la región paralela solo hay un hilo.
- No se puede llamar a ninguna función `MPI_…` antes de `MPI_Init` ni después de `MPI_Finalize`.

Constantes y funciones básicas (p17):
| Nombre | Qué es |
|---|---|
| `MPI_COMM_WORLD` | Comunicador con todos los procesos. |
| `MPI_MAX_PROCESSOR_NAME` | Tamaño máximo del nombre de la máquina (para declarar el arreglo). |
| `MPI_Get_processor_name(nombre, &largo)` | En qué máquina (nodo) corre este proceso. |
| `MPI_Wtime()` | Reloj en segundos (como `omp_get_wtime()`). |

### 6. Comunicación punto a punto bloqueante (p18–p20)
```cpp
int MPI_Send(const void* buf, int count, MPI_Datatype datatype, int dest,   int tag, MPI_Comm comm);
int MPI_Recv(void* buf,       int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm,
             MPI_Status* status);
```
| Argumento | Significa |
|---|---|
| `buf` | **Dirección** del dato (por eso `&x`; si es un arreglo, el arreglo). |
| `count` | **Cuántos elementos** (no bytes). |
| `datatype` | Tipo de cada elemento: `MPI_INT`, `MPI_DOUBLE`… (tabla de abajo). |
| `dest` / `source` | Rank del otro proceso. En `Recv` se puede usar `MPI_ANY_SOURCE`. |
| `tag` | Etiqueta (entero) para distinguir mensajes. **El tag de `Send` y el de `Recv` deben coincidir.** |
| `comm` | Comunicador (casi siempre `MPI_COMM_WORLD`). |
| `status` | (solo `Recv`) quién lo mandó, con qué tag, cuántos elementos llegaron. |

- **Bloqueante** = la función **no regresa** hasta que es seguro seguir:
  - `MPI_Recv` regresa cuando el mensaje **ya llegó** al buffer.
  - `MPI_Send` regresa cuando ya puedes **reusar** `buf` (el mensaje ya se copió o ya se entregó).
- ⚠️ **Deadlock:** si los dos procesos de una pareja hacen primero `MPI_Recv`, cada uno espera un
  mensaje que el otro nunca manda, y se quedan colgados para siempre. **Solución:** uno manda primero y
  el otro recibe primero (ejercicio de la p20), o usar comunicación no bloqueante.

**Tipos de datos MPI** (p21): `MPI_INT`↔`int`, `MPI_LONG`↔`long`, `MPI_LONG_LONG`↔`long long`,
`MPI_FLOAT`↔`float`, `MPI_DOUBLE`↔`double`, `MPI_UNSIGNED…`↔`unsigned …`, `MPI_BYTE`↔`char`.
Se indica el tipo porque MPI puede mandar entre máquinas distintas y necesita saber cómo interpretar
los bytes.

### 7. Comunicación punto a punto NO bloqueante (p22–p25)
```cpp
int MPI_Isend(const void* buf, int count, MPI_Datatype t, int dest,   int tag, MPI_Comm c, MPI_Request* req);
int MPI_Irecv(void* buf,       int count, MPI_Datatype t, int source, int tag, MPI_Comm c, MPI_Request* req);
int MPI_Wait(MPI_Request* req, MPI_Status* status);
```
- La **I** es de *immediate*: la función **regresa de inmediato** y solo "registra" la operación.
  Te da un `MPI_Request`, que es como un **recibo** para preguntar después por ella.
- `MPI_Wait(&req, &status)` **bloquea hasta que esa operación termine**.
- **Regla de oro:** entre el `Isend`/`Irecv` y su `Wait` **no toques el buffer**. No escribas en el de
  `Isend` (podría mandarse el valor nuevo) y no leas el de `Irecv` (podría no haber llegado nada todavía).
- **Ventajas:**
  1. **Evita deadlocks**: todos pueden "recibir primero", porque nadie se queda trabado en la llamada.
  2. **Traslapa comunicación con cómputo**: mientras el mensaje viaja, el proceso puede trabajar.

| | Bloqueante (`Send`/`Recv`) | No bloqueante (`Isend`/`Irecv` + `Wait`) |
|---|---|---|
| ¿Cuándo regresa? | Cuando la operación ya es segura | De inmediato |
| ¿Puedo usar el buffer después? | Sí | Solo después del `Wait` |
| Riesgo | Deadlock si el orden está mal | Leer un buffer antes de tiempo |
| Traslapar cómputo | No | Sí |

### 8. Comunicación colectiva (p15, p26–p29)
**Todos** los procesos del comunicador llaman **a la misma función** con el mismo `root`. No hay un
`Send` de un lado y un `Recv` del otro. Si un proceso no la llama, los demás se quedan esperando.

| Rutina | Qué hace (p15) | Firma |
|---|---|---|
| **Broadcast** | El root manda **el mismo dato** a todos. | `MPI_Bcast(&dato, count, tipo, root, comm)` |
| **Scatter** | El root **reparte** un arreglo: un pedazo distinto a cada proceso (incluido él mismo). | `MPI_Scatter(envio, n_por_proceso, tipo, recibo, n_por_proceso, tipo, root, comm)` |
| **Gather** | Lo contrario: el root **junta** un pedazo de cada proceso, en orden de rank. | `MPI_Gather(envio, n, tipo, recibo, n, tipo, root, comm)` |
| **Reduce** | El root recibe **un solo valor** que combina los de todos con una operación. | `MPI_Reduce(&mio, &resultado, count, tipo, op, root, comm)` |

- En `Scatter` y `Gather`, el `count` es **por proceso** (p27: "elementos a enviar / a recibir"), no
  el total. Con 3 procesos y vectores de 5: `count = 5`, no 15.
- En `Reduce`, el resultado solo es válido **en el root**; en los demás procesos no se toca.
- **Operaciones de `MPI_Reduce`** (p29): `MPI_SUM`, `MPI_PROD`, `MPI_MAX`, `MPI_MIN`, lógicas
  (`MPI_LAND`, `MPI_LOR`, `MPI_LXOR`) y de bits (`MPI_BAND`, `MPI_BOR`, `MPI_BXOR`).
- Es la versión de MPI de `reduction(+:suma)` de OpenMP (nota 02): cada uno acumula en lo suyo y al
  final se combina.

### 9. Medir tiempo (p32–p33)
- **En MPI:** `MPI_Wtime()`, solo en el proceso 0 (`if (id == 0) start = MPI_Wtime();`). Si el
  cronómetro se detiene **después de un `MPI_Reduce`**, el 0 ya esperó a que todos terminaran su parte.
- **En serial:** `chrono::high_resolution_clock::now()` y `duration_cast<microseconds>`.

## Comparaciones / tablas

### ¿Qué rutina uso?
| Necesito… | Rutina |
|---|---|
| Que todos sepan un parámetro que solo tiene el 0 (n, eps, el rango…) | `MPI_Bcast` |
| Repartir un arreglo grande en pedazos | `MPI_Scatter` |
| Juntar los resultados parciales en el 0 | `MPI_Gather` |
| Un total, máximo o mínimo de todos | `MPI_Reduce` (`MPI_SUM`, `MPI_MAX`, …) |
| Hablar solo con un vecino o pareja | `MPI_Send`/`MPI_Recv` (o `Isend`/`Irecv`) |

## Código de ejemplo visto en clase (ejercicios)

Todos se compilan con `mpic++ -std=c++17 -O2 <archivo>.cpp -o <archivo>.out` desde `ejercicios/`.

### Ejercicio 0 — Hola mundo (p8–p12) · `ejercicios/mpi_hello_world.cpp`
Cada proceso imprime su rank, el size y la máquina. Salida con `mpirun -n 4`:
```
Hola desde el proceso 1 de 4 en Monis-MacBook-Pro.local
Hola desde el proceso 0 de 4 en Monis-MacBook-Pro.local
Hola desde el proceso 3 de 4 en Monis-MacBook-Pro.local
Hola desde el proceso 2 de 4 en Monis-MacBook-Pro.local
```
- **Por qué salen en desorden:** los 4 procesos corren a la vez y cada uno escribe cuando puede.
  No hay un orden garantizado, igual que con los hilos de OpenMP.

### Ejercicio 1 — Interacción bloqueante (p20) · `ejercicios/mpi_bloqueante.cpp`
Con 8 procesos se forman parejas `i ↔ i+4` (0↔4, 1↔5, 2↔6, 3↔7). Cada uno manda su rank a su
pareja y recibe el de ella.

- `ejercicios/mpi_bloqueante.cpp:30-34` — la **primera mitad** hace `Send` (tag 1) y luego `Recv` (tag 2).
- `ejercicios/mpi_bloqueante.cpp:35-42` — la **segunda mitad** hace lo contrario: `Recv` (tag 1) y
  luego `Send` (tag 2). **Este orden cruzado es lo que evita el deadlock.**
- Los tags 1 y 2 vienen de la p17: el mensaje de "ida" usa tag 1 y el de "vuelta" usa tag 2.

Salida con `mpirun -n 8`:
```
Proceso 2: mi pareja es 6 y me mando el mensaje 6
Proceso 6: mi pareja es 2 y me mando el mensaje 2
Proceso 4: mi pareja es 0 y me mando el mensaje 0
Proceso 0: mi pareja es 4 y me mando el mensaje 4
Proceso 5: mi pareja es 1 y me mando el mensaje 1
Proceso 1: mi pareja es 5 y me mando el mensaje 5
Proceso 7: mi pareja es 3 y me mando el mensaje 3
Proceso 3: mi pareja es 7 y me mando el mensaje 7
```

### Ejercicio 2 — Anillo no bloqueante (p25) · `ejercicios/mpi_anillo_no_bloqueante.cpp`
Los procesos forman un anillo (el último es vecino del 0). Cada uno manda su rank a sus dos vecinos y
recibe el de ambos.

- `ejercicios/mpi_anillo_no_bloqueante.cpp:24-25` — vecinos: `izq = (id - 1 + n) % n` y
  `der = (id + 1) % n`. El `+ n` es para que el vecino izquierdo del 0 sea `n-1` y no `-1`.
- `ejercicios/mpi_anillo_no_bloqueante.cpp:30-33` — **4 operaciones no bloqueantes**: 2 `Irecv`
  (uno por vecino, cada uno en **su propio buffer**) y 2 `Isend`. Todos empiezan recibiendo y aun
  así no hay deadlock, porque `Irecv` no se queda esperando.
- Tags: 1 = "el mensaje viaja hacia la derecha", 2 = "hacia la izquierda". Sin tags distintos, con 2
  procesos (donde izq = der) no se sabría cuál mensaje es cuál.
- `ejercicios/mpi_anillo_no_bloqueante.cpp:39-40` — `MPI_Wait` de los 4 requests **antes** de leer
  `recibido_izq` y `recibido_der`.

Salida con `mpirun -n 6`:
```
Proceso 4: vecino izq 3 me mando 3, vecino der 5 me mando 5
Proceso 3: vecino izq 2 me mando 2, vecino der 4 me mando 4
Proceso 5: vecino izq 4 me mando 4, vecino der 0 me mando 0
Proceso 1: vecino izq 0 me mando 0, vecino der 2 me mando 2
Proceso 0: vecino izq 5 me mando 5, vecino der 1 me mando 1
Proceso 2: vecino izq 1 me mando 1, vecino der 3 me mando 3
```

### Ejercicio 3 — Bcast, Scatter, Gather y Reduce (p26–p29) · `ejercicios/mpi_scatter_gather.cpp`
Usa los datos de la p27: 3 vectores de 5 floats. `Bcast` del tamaño del vector, `Scatter` de un
vector a cada proceso, promedio local, `Gather` de los promedios en el 0 y `Reduce` con `MPI_SUM`
y `MPI_MAX`.

Salida con `mpirun -n 3` (los promedios deben dar 3, 8 y 13):
```
Proceso 0: recibi [1 .. 5], promedio = 3
Proceso 1: recibi [6 .. 10], promedio = 8
Proceso 2: recibi [11 .. 15], promedio = 13
ROOT junto los promedios (Gather): 3 8 13
ROOT: suma de promedios (Reduce MPI_SUM) = 24, maximo (Reduce MPI_MAX) = 13
```

### Ejercicio 4 — Números primos (p30–p38) · `ejercicios/mpi_primos.cpp`
Contar los primos en `[2, 10^n]` para n = 1…9 e imprimir `límite  cantidad  segundos` (p31).

Cómo está armado (siguiendo los tips de la p34–p37):
1. **`MPI_Bcast`** del límite del rango (10, 100, …): solo el 0 lo calcula y se lo avisa a todos
   (`ejercicios/mpi_primos.cpp:43`).
2. **Reparto cíclico** (p36): el proceso `id` revisa `2+id, 2+id+P, 2+id+2P, …`
   (`ejercicios/mpi_primos.cpp:46-48`). Con 4 procesos: el 0 revisa 2, 6, 10…; el 1 revisa 3, 7, 11…
3. **Primalidad** (p35): buscar un divisor de 2 a `√n` con `%` (`ejercicios/mpi_primos.cpp:18-23`).
   Basta con llegar a la raíz porque si `n = a·b` con `a ≤ b`, entonces `a ≤ √n`.
4. **`MPI_Reduce` con `MPI_SUM`** de los conteos locales (`ejercicios/mpi_primos.cpp:52`).
5. **`MPI_Wtime`** solo en el 0, antes del `Bcast` y después del `Reduce` (p32).

Base serial para el speedup: `ejercicios/primos_serial.cpp` (misma prueba de primalidad, cronómetro
con `chrono` como en la p33). Se compila con `clang++ -std=c++17 -O2`.

**Conteos verificados** contra la tabla de la p30 (serial, hasta 10^7): 4, 25, 168, 1229, 9592,
78498, 664579 ✅.

**Tiempos medidos en tu Mac** (i9-9880H: 8 cores físicos / 16 lógicos; **una sola corrida** por
configuración, así que tómalos como orientativos). Con 16 y 32 procesos se usó `--oversubscribe`:

| Rango | Serial (s) | MPI P=8 (s) | MPI P=16 (s) | MPI P=32 (s) | Speedup P=8 / 16 / 32 |
|---|---|---|---|---|---|
| [2, 10] | 0.000000 | 0.000179 | 0.000173 | 0.000509 | ≈ 0 (MPI más lento) |
| [2, 10^4] | 0.00031 | 0.000102 | 0.000087 | 0.000276 | 3.0 / 3.6 / 1.1 |
| [2, 10^6] | 0.150 | 0.0389 | 0.0289 | 0.0194 | 3.9 / 5.2 / 7.8 |
| [2, 10^7] | 3.85 | 0.964 | 0.696 | 0.442 | 4.0 / 5.5 / 8.7 |
| [2, 10^8] | 102.8 | 27.0 | 15.1 | 12.1 | **3.8 / 6.8 / 8.5** |

Todos los conteos coinciden con la p30 hasta 10^8 (5,761,455). 10^9 no se corrió: el serial tardaría
más de una hora.

**Cómo leerlo** (igual que la gráfica de la p38):
- **Rangos chicos:** MPI es **más lento** que el serial. Lanzar el `Bcast` y el `Reduce` cuesta décimas de
  milisegundo, más que revisar 10 números. Es la misma idea que la granularidad fina de la nota 01.
- **Rangos grandes:** el speedup sube porque el cálculo domina sobre la comunicación (solo hay
  **2 mensajes colectivos** por rango, sin importar su tamaño).
- **Con P=8 da solo 3.8x**, aunque hay 8 cores físicos. La causa es el desbalance que se explica
  abajo: solo 4 de los 8 procesos trabajan.

#### Hallazgo: el reparto cíclico desperdicia la mitad de los procesos
Con un número **par** de procesos P, el proceso `id` solo revisa números de la forma `2+id+kP`.
Todos tienen la **misma paridad** que `2+id`:
- Los procesos de `id` **par** solo reciben números **pares**. Se descartan en la primera división
  (`% 2 == 0`), así que **casi no trabajan**.
- Los de `id` **impar** reciben **todos** los impares, que son los caros.

Lo medí contando cuántas divisiones (`%`) le tocan a cada proceso hasta 10^7:

| P | % del trabajo por proceso (ranks 0, 1, 2, 3…) | Speedup máximo posible |
|---|---|---|
| 4 | 0.1%, 49.9%, 0.1%, 49.9% | 2.0 |
| 8 | 0.1%, 24.9%, 0.1%, 24.9%, … | 4.0 |
| 16 | 0.0%, 12.5%, 0.0%, 12.5%, … | 8.0 |
| 32 | 0.0%, 6.2%, 0.0%, 6.2%, … | 16.0 |

**El speedup no puede pasar de P/2.** Es un problema de **balance de carga** (como `static` vs
`dynamic` en la nota 02): el trabajo está repartido en **cantidad de números** de forma pareja, pero no
en **costo**.

**Arreglo** (`ejercicios/mpi_primos_balanceado.cpp`, no viene en las diapositivas): el 0 cuenta el 2
aparte y solo se reparten los **impares**: `3+2·id, 3+2·id+2P, …`. Así cada proceso recibe la misma
mezcla de números.

Medido (mismas condiciones; una corrida):

| P | Original 10^8 (s) | Balanceado 10^8 (s) | Speedup original | Speedup balanceado | Procesos que **sí** trabajan (orig. / bal.) |
|---|---|---|---|---|---|
| 8 | 27.0 | 16.8 | 3.8 | **6.1** | 4 / 8 |
| 16 | 15.1 | 15.0 | 6.8 | 6.8 | 8 / 16 |
| 32 | 12.1 | 16.8 | **8.5** | 6.1 | 16 / 32 |

- **P=8:** el balanceado gana claramente (3.8x → 6.1x), porque usa los 8 cores físicos en vez de 4.
- **P=16 y P=32:** ya no ayuda, y con 32 hasta empeora. Lo que importa es **cuántos procesos trabajan
  de verdad** contra **cuántos cores hay**:
  - 8 procesos trabajando en 8 cores físicos → ~6–7x.
  - 16 trabajando en 16 cores lógicos → Hyper-Threading, que da poco extra (nota 01).
  - 32 trabajando en 16 cores lógicos → **sobresuscripción**: el sistema operativo turna los procesos
    y se pierde tiempo en cambios de contexto. El original con P=32 en realidad tiene "solo" 16
    procesos ocupados, y por eso le va mejor.
- ⚠️ Pendiente (hipótesis no verificada): Open MPI suele esperar en `MPI_Reduce` **revisando
  activamente** (*busy-polling*). Si es así, los procesos que ya acabaron también consumen CPU mientras
  esperan, y eso podría explicar parte de las diferencias.
- **Conclusión:** balancear importa cuando **procesos ≤ cores físicos**, que es el caso que conviene
  usar. Con más procesos que cores, el cuello de botella pasa a ser el hardware.

## Preguntas tipo examen
1. ¿Cuál es la diferencia fundamental entre MPI y OpenMP? (procesos con memoria propia y mensajes vs
   hilos con memoria compartida).
2. ¿Qué es un comunicador? ¿Qué es el rank? ¿Puede un proceso tener dos ranks distintos? (Sí, uno por
   comunicador, p13.)
3. Explica cada argumento de `MPI_Send` y `MPI_Recv`. ¿`count` está en bytes o en elementos? (Elementos.)
4. ¿Qué significa que `MPI_Recv` sea bloqueante? ¿Cuándo regresa `MPI_Send`?
5. Dos procesos hacen primero `MPI_Recv` y luego `MPI_Send`. ¿Qué pasa? (Deadlock.) ¿Cómo se arregla?
6. ¿Qué es un `MPI_Request`? ¿Qué pasa si leo el buffer de un `MPI_Irecv` antes del `MPI_Wait`?
7. ¿Por qué en el anillo no hay deadlock aunque todos empiezan recibiendo?
8. Diferencia entre `Bcast` y `Scatter`. ¿Y entre `Gather` y `Reduce`?
9. En `MPI_Scatter` con 3 procesos y un arreglo de 15 floats, ¿qué `count` pongo? (5, es por proceso.)
10. ¿En qué proceso queda el resultado de `MPI_Reduce`? (Solo en el root.)
11. En el ejercicio de primos, ¿qué rutinas usarías y para qué? (`Bcast` del rango y `Reduce` con
    `MPI_SUM` de los conteos.)
12. ¿Por qué basta revisar divisores hasta `√n`?
13. ¿Por qué el speedup de primos con 32 procesos es tan bajo (p38: ~5x)? (Reparto cíclico: los de rank
    par solo reciben pares → máximo P/2. Además, hay más procesos que cores y se sobresuscribe la máquina.)
14. ¿Por qué el speedup es 0 en `[2, 10]`, `[2, 100]` y `[2, 1000]` (p38)? (El trabajo es tan poco que
    arrancar la comunicación cuesta más que calcular.)
15. ¿Por qué `mpirun -n 16` falla en tu Mac y cómo se arregla? (8 cores físicos = 8 "slots";
    `--oversubscribe`.)

## Dudas resueltas en conversación con Claude
- **¿`openmpi` u `open-mpi`?** En Homebrew la fórmula es `open-mpi`; `openmpi` (como en la p6) funciona
  como alias.
- **¿`mpiexec` o `mpirun`?** En Open MPI son el mismo programa; las diapositivas de Windows usan `mpiexec`.
