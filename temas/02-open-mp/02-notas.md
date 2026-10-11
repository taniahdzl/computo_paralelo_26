# 02 — OpenMP (y lo básico de C++ que necesitas)

> Fuente: `OpenMP.pdf` (74 diapositivas). Entre paréntesis va la página del PDF, p. ej. (p35).
> Todos los ejemplos de esta nota se compilaron y corrieron en tu Mac; las salidas son reales.

## Cómo compilar en tu Mac (¡distinto al de las diapositivas!)
- **Diapositivas (Windows/Linux, g++ de verdad):** `g++ holamundo.cpp -o holamundo -fopenmp`
- **Tu Mac:** `g++` en realidad es Apple clang, que **no acepta `-fopenmp` directo**. Hay que:
  ```bash
  clang++ -std=c++17 -O2 -Xpreprocessor -fopenmp \
      -I/usr/local/opt/libomp/include -L/usr/local/opt/libomp/lib -lomp \
      archivo.cpp -o archivo.out
  ```
  - `-Xpreprocessor -fopenmp`: le pasa la bandera de OpenMP al preprocesador (es quien entiende los `#pragma`).
  - `-I…/include`: dónde está `omp.h`.
  - `-L…/lib -lomp`: enlaza la librería `libomp` (instalada con Homebrew).
- **En VS Code:** abre el `.cpp` y presiona **Cmd+Shift+B**; ya está configurado en `.vscode/tasks.json`.
- **Correr con cierto número de hilos:** `OMP_NUM_THREADS=4 ./archivo.out`

---

## Parte A — C++ "no tan básico" (p3–p14)

### Apuntadores y arreglos dinámicos
- Un **apuntador** (`int*`) guarda una **dirección de memoria**, no un valor.
- `new` pide memoria en tiempo de ejecución (en el *heap*); `delete[]` la devuelve. **Todo `new[]`
  necesita su `delete[]`**, o hay fuga de memoria.
- ¿Por qué arreglos dinámicos y no `int a[200000]`? Los arreglos normales viven en el **stack**, que
  es chico (2–8 MB, p25). Con muchos puntos truena (*stack overflow*). El heap es mucho más grande.

```cpp
int* arreglo {new int[5]{1, 2, 3, 4, 5}};  // reserva 5 enteros en el heap
// ... usar arreglo[0] ... arreglo[4]
delete[] arreglo;                          // libera (con [] porque fue new[])
```

### Matriz dinámica (p12–p13)
```cpp
int** matriz {new int*[ren]};       // 1) arreglo de 'ren' apuntadores (uno por renglón)
for (int i = 0; i < ren; i++)
    matriz[i] = new int[col];       // 2) cada renglón es su propio arreglo de 'col' enteros

for (int i = 0; i < ren; i++)
    delete[] matriz[i];             // liberar: primero cada renglón...
delete[] matriz;                    // ...y al final el arreglo de apuntadores
```
En la p13 se ve que dentro de un renglón las direcciones van de 4 en 4 (`…e50, …e54, …e58`, porque un
`int` mide 4 bytes), pero el renglón 1 está en otro lugar (`…f00`). **Los renglones no son contiguos
entre sí.**

### Argumentos desde la terminal (p14)
```cpp
int main(int argc, char** argv) {
    // argc = cuantos argumentos hay (incluye el nombre del programa)
    // argv[0] = nombre del programa, argv[1] = primer argumento, ...
    int n       = atoi(argv[1]);   // texto -> int
    long grande = atol(argv[2]);   // texto -> long
    double eps  = atof(argv[3]);   // texto -> double
}
```
Así recibe tu `dbscan_serial` el CSV, `eps` y `min_samples`.

### Números aleatorios (p74)
```cpp
random_device rd;                           // semilla "real" (del sistema)
mt19937 gen(rd());                          // motor Mersenne Twister
uniform_int_distribution<> dis(1, 1000);    // enteros uniformes entre 1 y 1000
int x = dis(gen);
```

---

## Parte B — OpenMP

### ¿Qué es? (p15–p20)
- **Open specifications for Multi-Processing**: un estándar para paralelizar en **memoria compartida**
  (UMA o NUMA) con **hilos**.
- **Modelo fork-join** (p17): el programa empieza con un solo hilo (el **maestro**, id 0). Al llegar a
  una región paralela, hace **fork** (crea un equipo de hilos); al terminar la región, hace **join**
  (todos esperan, se unen al maestro y queda uno solo).
  ```
  maestro ──●──[ hilos 0,1,2,3 trabajando ]──●── maestro ──●──[ ... ]──●──
           fork                             join           fork       join
  ```
- Es **SPMD**: todos los hilos corren el mismo código, pero cada uno puede tomar datos distintos o
  caminos distintos (según su id).
- **Paralelismo explícito**: tú decides qué se paraleliza.
- **OpenMP NO revisa por ti** (p20): dependencias de datos, condiciones de carrera ni deadlocks.
  **Que el programa sea correcto es tu responsabilidad.**

### Las 3 piezas de OpenMP (p21–p24)
| Pieza | Para qué | Ejemplo |
|---|---|---|
| **Directivas** (`#pragma`) | Crear regiones paralelas, repartir trabajo, sincronizar | `#pragma omp parallel for` |
| **Funciones de librería** (`omp.h`) | Preguntar o ajustar número de hilos, id, tiempo | `omp_get_thread_num()` |
| **Variables de ambiente** | Configurar desde fuera sin recompilar | `export OMP_NUM_THREADS=8` |

Formato de una directiva (p22): `#pragma omp <constructo> [cláusulas…]`. **Distingue mayúsculas y
minúsculas.**

### Funciones que más vas a usar (p28–p29)
| Función | Regresa |
|---|---|
| `omp_get_thread_num()` | El id de **este** hilo (0 = maestro … N-1). |
| `omp_get_num_threads()` | Cuántos hilos hay **en el equipo actual**. Fuera de una región paralela regresa **1**. |
| `omp_set_num_threads(n)` | Fija cuántos hilos usar en las siguientes regiones. |
| `omp_get_max_threads()` | Cuántos hilos usaría la próxima región. |
| `omp_get_wtime()` | Tiempo de reloj de pared en segundos (para medir: `fin - inicio`). |

**¿Cuántos hilos se usan?** (p28). Hay tres formas, de mayor a menor prioridad: la cláusula
`num_threads(n)` en el pragma, `omp_set_num_threads(n)` en el código y la variable de ambiente
`OMP_NUM_THREADS`. Si no pones nada, se usa el número de cores virtuales (16 en tu Mac).

### Estructura general de un programa (p26–p27)
```cpp
#include <omp.h>
int main() {
    int var1, var2, var3;
    // codigo serial (solo el maestro)
    #pragma omp parallel private(var1, var2) shared(var3)
    {
        // codigo que ejecutan TODOS los hilos
    }   // <- aqui todos se unen al maestro (join, barrera implicita)
    // codigo serial otra vez
}
```

Cláusulas de `parallel` (p27):
| Cláusula | Significa |
|---|---|
| `shared(x)` | Todos los hilos ven **la misma** `x`. ⚠️ Si escriben en ella → posible carrera. |
| `private(x)` | Cada hilo tiene **su propia copia** de `x`, **sin inicializar** (basura). |
| `firstprivate(x)` | Copia propia por hilo, **inicializada** con el valor que `x` tenía antes de la región. |
| `default(shared \| none)` | Regla por omisión. Con `none` te obliga a declarar cada variable (útil para no equivocarte). |
| `num_threads(n)` | Cuántos hilos para **esta** región. |
| `if(condición)` | Solo paraleliza si la condición es verdadera (p. ej. `if(n > 10000)`). |

**Reglas automáticas importantes:**
- Las variables declaradas **antes** de la región son **compartidas** por default.
- Las variables declaradas **dentro** de la región son **privadas** (viven en el stack de cada hilo).
- El índice de un `for` paralelizado (`i`) es **privado** automáticamente.

### Mi primer programa paralelo (p31), línea por línea
```cpp
#include <omp.h>
#include <iostream>

int main() {
    int nthreads, thread_id;                 // declaradas fuera -> compartidas por default...

    #pragma omp parallel private(thread_id)  // ...pero thread_id se vuelve privada: una por hilo
    {                                        // FORK: aqui nacen los hilos
        thread_id = omp_get_thread_num();    // cada hilo guarda SU id en SU copia
        std::cout << "Hola desde el hilo " << thread_id << "\n";

        if (thread_id == 0) {                // solo el maestro entra aqui
            nthreads = omp_get_num_threads();
            std::cout << "Número de hilos: " << nthreads << "\n";
        }
    }                                        // JOIN: todos esperan y se unen al maestro
}
```
**Pregunta típica:** ¿por qué los "Hola" salen en desorden y a veces mezclados? Porque los hilos
corren al mismo tiempo, no hay un orden garantizado y `cout` es un recurso compartido.
**¿Qué pasa si `thread_id` no fuera `private`?** Todos escribirían en la misma variable (carrera) y un
hilo podría imprimir el id de otro.

### Fors paralelos (p32–p38), lo más importante
Con 10 iteraciones y 4 hilos:

| Código | Qué pasa |
|---|---|
| `for` normal | El hilo 0 hace las 10 iteraciones. |
| `#pragma omp parallel for` + `for` | ✅ Las 10 iteraciones se **reparten**: hilo 0 → 0,1,2; hilo 1 → 3,4,5; hilo 2 → 6,7; hilo 3 → 8,9. |
| `#pragma omp parallel { #pragma omp for … }` | ✅ Lo mismo, en dos pasos: `parallel` crea los hilos y `for` reparte. Útil si quieres varios `for` dentro de **una sola** región (sin crear y destruir hilos cada vez). |
| ❌ `#pragma omp parallel` + `for` (sin `for` en el pragma) | **Cada hilo hace el for COMPLETO**: el trabajo se repite 4 veces (p37). |
| ❌ `#pragma omp for` sin `parallel` alrededor | No hay equipo de hilos, así que **lo hace uno solo** (p38). |

**Para que un for se pueda paralelizar**, las iteraciones deben ser **independientes**: ninguna lee
algo que otra iteración escribe.

### `schedule`: cómo se reparten las iteraciones (p33, p40–p46)
| Opción | Cómo reparte | Cuándo conviene |
|---|---|---|
| (sin schedule) | En tu Mac: **bloques contiguos iguales**. Con 16 iteraciones y 4 hilos: `0 0 0 0 1 1 1 1 2 2 2 2 3 3 3 3` (lo comprobé). | Todas las iteraciones tardan lo mismo. |
| `schedule(static, c)` | Pedazos de `c` iteraciones repartidos **por turnos, decidido desde antes**. Con `c = 1`: `0 1 2 3 0 1 2 3 …` (lo comprobé). | Trabajo parejo; costo de reparto casi cero. |
| `schedule(dynamic, c)` | Cada hilo toma `c` iteraciones; **cuando acaba, pide más**. | Iteraciones que **tardan distinto** (desbalance). |

- Las p40 y p43 muestran el problema: con reparto en bloques, si unas iteraciones tardan más, unos
  hilos terminan y **se quedan esperando** a otro (desequilibrio de carga).
- `dynamic` lo arregla (p44), pero **cuesta**: pedir trabajo requiere sincronizar. Con `chunk` muy
  chico ese costo crece, y con `chunk` muy grande vuelve el desbalance. **Hay que experimentar**
  (p45–p46: el experimento de chunk sizes 1…1280 con hilos {1, max/2, max}).

### Fors anidados (p47–p52)
Con 3×6 = 18 trabajos y 4 hilos:

| Código | Resultado |
|---|---|
| `parallel for` solo en el `for` de afuera (3 iteraciones) | ❌ Solo 3 hilos trabajan y **el hilo 3 se queda sin nada** (p48). |
| `parallel for` en el `for` de adentro | Usa los 4 hilos, pero **crea y destruye el equipo 3 veces** (overhead) (p49). |
| `parallel for` en los **dos** fors ("el engaño", p50) | ❌ Por default el paralelismo anidado está apagado: el de adentro crea un equipo de 1 hilo, así que da lo mismo que la primera fila. |
| ✅ Un solo `for` aplanado (p51) | `for (ij = 0; ij < 3*6; ij++) c(ij / 6, ij % 6);` → 18 iteraciones reales. |
| ✅ `collapse(2)` (p52) | `#pragma omp parallel for collapse(2)` hace el aplanado por ti. **Los fors deben ser "rectangulares"** (el de adentro no depende de `i`). |

**Floyd-Warshall** (p53–p62): rutas más cortas entre todos los pares de nodos. El `for` de `k` (nodo
intermedio) **no** se puede paralelizar, porque cada `k` usa la matriz que dejó el `k` anterior. Los
`for` de `i` y `j` **sí** (dentro de un mismo `k`). Es un buen ejemplo de restricción de orden.

### Sincronización (p63–p71)
| Directiva | Qué hace |
|---|---|
| `#pragma omp critical (nombre)` | Solo **un hilo a la vez** entra al bloque. El `nombre` es el **candado**. |
| `#pragma omp atomic` | Una sola operación simple (`x = x + 1`, `x += …`) se hace "de un jalón" a nivel máquina. **Más rápido que critical**, pero solo para una instrucción. |
| `#pragma omp master` | Solo el hilo 0 ejecuta el bloque. **Sin barrera** (los demás no lo esperan). |
| `#pragma omp single` | Solo **un** hilo (cualquiera) lo ejecuta; los demás **esperan al final** (barrera implícita), a menos que pongas `nowait`. |
| `#pragma omp barrier` | Todos esperan aquí hasta que lleguen todos. |
| `#pragma omp sections` / `section` | Bloques **distintos** de código, cada uno a un hilo (paralelismo de tareas). Barrera al final salvo `nowait`. |

⚠️ **Trampa del `critical` con nombre** (p64–p65):
- Regiones con el **mismo nombre**, o sin nombre, se excluyen entre sí (comparten candado).
- Regiones con **nombres distintos** **no** se excluyen: son candados distintos.
- En el ejemplo de la p64, `region_uno` y `region_dos` modifican la misma `x`. Como tienen nombres
  distintos, un hilo puede estar en `region_uno` mientras otro está en `region_dos`, y **los dos
  escriben `x` a la vez: hay carrera.**
- **Lo comprobé** (16 hilos, cada región suma 100,000): esperado 3,200,000; salió
  `3200000, 3200000, 3000000, 2800000, 2800000` en 5 corridas. **A veces bien y a veces mal:** así se
  ven las carreras.

### `reduction` (p72–p73)
Problema: sumar en paralelo en una sola variable `suma` es una carrera. La solución con `critical`
sería correcta pero lentísima (todos en fila).

`reduction(+:suma)` hace esto:
1. Cada hilo tiene su **copia privada** de `suma`, inicializada con el **neutro** del operador.
2. Cada hilo suma su parte en **su** copia (sin pelear con nadie).
3. Al final, OpenMP **combina** las copias con el operador y deja el total en `suma`.

| Operador | Valor inicial de cada copia |
|---|---|
| `+`, `-`, `\|`, `^`, `\|\|` | 0 |
| `*`, `&&` | 1 |
| `&` | ~0 (todos los bits en 1) |
| `max` | El número **más pequeño** representable |
| `min` | El número **más grande** representable |

**Lo comprobé** (sumar 1…1,000,000):
```
sin reduction: 486328593750    <- incorrecto (carrera), y cambia en cada corrida
con reduction: 500000500000    <- correcto
esperado:      500000500000
```

## Comparaciones / tablas

### `critical` vs `atomic` vs `reduction`
| | Protege | Costo | Úsalo cuando |
|---|---|---|---|
| `critical` | Un bloque de cualquier tamaño | Alto (candado) | Varias instrucciones que deben ir juntas. |
| `atomic` | Una operación simple | Bajo | Un `x++` o `x += …` aislado. |
| `reduction` | Un acumulador (+, *, max…) | Mínimo | Sumar, contar o sacar el máximo en un `for`. **Primera opción.** |

### `master` vs `single`
| | ¿Quién lo ejecuta? | ¿Los demás esperan? |
|---|---|---|
| `master` | Siempre el hilo 0 | No |
| `single` | El primero que llegue | Sí (salvo `nowait`) |

## Código de ejemplo visto en clase

### Ejercicio: suma de vectores (p39), línea por línea
```cpp
#include <iostream>
#include <cstdlib>   // atoi
#include <omp.h>
using namespace std;

int main(int argc, char** argv) {
    int n = atoi(argv[1]);            // tamano desde la terminal

    float* a = new float[n];          // heap, no stack: n puede ser enorme
    float* b = new float[n];
    float* c = new float[n];

    // Inicializacion: cada i es independiente -> se puede paralelizar
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        a[i] = b[i] = i * 1.0f;
    }

    double inicio = omp_get_wtime();  // empezar a medir SOLO lo que nos interesa

    // Suma: c[i] solo depende de a[i] y b[i] -> sin carreras.
    // Cada hilo escribe posiciones DISTINTAS de c, por eso no necesita critical.
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }

    double fin = omp_get_wtime();
    cout << "c[n-1] = " << c[n - 1] << "  tiempo = " << fin - inicio << " s\n";

    delete[] a; delete[] b; delete[] c;
    return 0;
}
```
Tiempos reales en tu Mac (n = 100,000,000, 3 corridas por número de hilos):

| Hilos | Tiempo (s) |
|---|---|
| 1 | 0.23 – 0.30 |
| 2 | ~0.13 |
| 4 | 0.08 – 0.13 |
| 8 | 0.065 – 0.077 |
| 16 | 0.063 – 0.073 |

**Cómo interpretarlo (muy de examen):**
- Mejora hasta ~4x y **se estanca desde 8 hilos**. No llega a 16x porque cada iteración casi no
  calcula (una suma) y sí mueve mucha memoria (2 lecturas y 1 escritura). Está **limitado por el
  ancho de banda de memoria** (nota 01, sección 13). Es granularidad fina.
- Los tiempos varían entre corridas, sobre todo con 1 hilo. Por eso **hay que promediar varias
  ejecuciones** (el proyecto pide 10).
- DBSCAN es lo contrario: por cada punto hace **n** cálculos de distancia sobre datos que caben en
  caché. Es mucho cómputo por dato, así que **debería escalar mucho mejor**.

## Preguntas tipo examen
1. Explica el modelo fork-join con un dibujo.
2. Diferencia entre `#pragma omp parallel` y `#pragma omp parallel for`. ¿Qué pasa si pongo solo
   `parallel` sobre un for?
3. `private` vs `firstprivate` vs `shared`. ¿Qué valor tiene una variable `private` al entrar a la región?
4. ¿Qué imprime `omp_get_num_threads()` fuera de una región paralela? (1)
5. Tengo iteraciones que tardan muy distinto: ¿`static` o `dynamic`? ¿Qué pasa con `chunk = 1` vs `chunk = 1280`?
6. Tengo dos fors anidados de 3×6 y 16 hilos. ¿Dónde pongo el pragma? (`collapse(2)` o aplanar).
7. ¿Por qué en Floyd-Warshall no se puede paralelizar el `for` de `k`?
8. Dos `critical` con nombres distintos modifican la misma variable: ¿hay carrera? (Sí.)
9. Escribe un `for` que sume un arreglo en paralelo **correctamente** (→ `reduction(+:suma)`).
10. ¿Qué hace `nowait`? ¿Qué directivas tienen barrera implícita? (fin de `parallel`, `for`, `single`, `sections`).
11. ¿Por qué usar `new` en vez de `float a[n]` para arreglos grandes? (stack chico, p25).

## Dudas resueltas en conversación con Claude
- **¿Por qué en Mac no funciona `g++ -fopenmp`?** Porque `g++` en macOS es Apple clang, que trae
  OpenMP desactivado. Se instala `libomp` con Homebrew y se compila con `-Xpreprocessor -fopenmp … -lomp`.
  En Linux, o en la máquina del profesor, el comando de las diapositivas funciona normal.
- **¿Por qué la suma de vectores no da 16x con 16 hilos?** Ancho de banda de memoria (ver la tabla de
  arriba). Y con 16 hilos en 8 cores físicos, los pares de hilos comparten core (Hyper-Threading).
