# Descripción del código

Proyecto Clausura: Bolsa de palabras con MPI. Equipo: Tania Hernandez y Tamara Schnaas.

## 1. Qué hace el programa

Dado un listado de k libros (URLs de Project Gutenberg o rutas locales) construye la **matriz de
bolsa de palabras**: una fila por libro, una columna por palabra del vocabulario (la unión de
las palabras de todos los libros, en orden alfabético), y en cada celda cuántas veces aparece esa
palabra en ese libro. La escribe en un CSV: primera fila el vocabulario, luego una fila por libro.

Hay dos programas que construyen exactamente la misma matriz:

| Archivo | Qué es |
|---|---|
| [`src/serial/bolsa_serial.cpp`](../src/serial/bolsa_serial.cpp) | Versión serial: descarga y cuenta los k libros uno por uno |
| [`src/mpi/bolsa_mpi.cpp`](../src/mpi/bolsa_mpi.cpp) | Versión MPI: en una sola corrida, corre la serial (para medir `T_serial`) y luego la paralela con q procesos (para medir `T_paralelo`), compara las dos matrices e imprime el speedup |
| [`src/comun/bolsa_comun.h`](../src/comun/bolsa_comun.h) | Funciones que comparten las dos versiones (descarga, limpieza y conteo de palabras, vocabulario, CSV) |

Que las dos versiones usen las mismas funciones de `bolsa_comun.h` para leer, limpiar, contar y
escribir es la razón por la que sus CSV son comparables byte a byte: la única diferencia entre
"serial" y "paralelo" es cómo se reparte el trabajo entre procesos, no cómo se cuenta cada palabra.

## 2. Reglas de limpieza y conteo (`bolsa_comun.h`)

Una palabra es una secuencia de **2 o más caracteres `a-z` o `0-9`, solo ASCII**, después de pasar
todo a minúsculas. Cualquier otro byte (espacios, signos de puntuación, `_`, letras con acento)
cierra la palabra actual y no se cuenta. No se quitan palabras vacías (*stopwords*): el enunciado
no las menciona (ver la tabla de decisiones abiertas en el [README](../README.md)).

`data/validar_sklearn.py` genera la misma matriz con `CountVectorizer` de sklearn, usando
`token_pattern=r"[a-z0-9]{2,}"` para que coincida exactamente con esta regla (el patrón por
defecto de sklearn también acepta `_` y letras con acento, y Gutenberg usa `_así_` para cursivas,
así que con el patrón por defecto la comparación nunca habría coincidido). Con esta regla, tanto
`bolsa_serial` como `bolsa_mpi` dan matrices **idénticas** a las de sklearn (probado con los 6
libros de `data/urls_locales.txt`, 23,655 palabras de vocabulario).

`contarPalabras` recorre el texto **una sola vez**, acumulando caracteres válidos y cerrando la
palabra actual al encontrar cualquier otro byte. Como `contarPalabrasEn` **suma** sobre un
`map<string,int>` que ya puede tener conteos (no lo reinicia), sirve para contar un libro **por
partes**: si el texto se corta en un byte que no es `a-z`/`0-9` (nunca a la mitad de una palabra),
contar cada parte por separado y sumar da *exactamente* el mismo resultado que contar el texto
completo de un jalón. Esta propiedad es la que hace correcto el reparto por trozos de la versión
MPI (sección 4).

## 3. Descarga

`obtenerTexto(fuente, indice)` decide por el prefijo: si `fuente` empieza con `http`, descarga con
`curl` (vía `std::system`) a un archivo temporal `libro_<indice>.txt` y lo borra después de leerlo;
si no, trata `fuente` como una ruta local (así se puede probar y medir sin depender de la red,
con `data/urls_locales.txt` generado por `data/preparar_libros_locales.sh`).

El índice que nombra el archivo temporal es el **índice global** del libro (su posición en
`urls.txt`), no un contador local de cada proceso: si cada proceso MPI numerara sus descargas
desde 0, dos procesos podrían escribir al mismo tiempo `libro_0.txt` y pisarse el archivo.

Antes de pasar la URL a un shell (`std::system`), se valida que solo tenga caracteres de una lista
segura (alfanuméricos y `: / . - _ ~ % ? = &`) para evitar inyección de comandos; cualquier otro
carácter se rechaza. `curl` se llama sin reintentos **a propósito**: Gutenberg bloquea
temporalmente (503, con `Retry-After` de hasta varias horas) a quien le hace muchas peticiones
automáticas, e insistir alarga el bloqueo.

## 4. Versión MPI (`bolsa_mpi.cpp`)

En una sola corrida (`mpirun -np q ./bolsa_mpi urls.txt salida.csv`) pasan tres cosas:

1. **El proceso 0 corre la versión serial completa** (descarga + conteo + CSV, en
   `salida.csv.serial.csv`) y mide `T_serial` con `MPI_Wtime()`.
2. **Todos los procesos corren la versión paralela** (abajo) y se mide `T_paralelo`.
3. **El proceso 0 compara** las dos matrices (`conteos_serial == conteos_par`, comparación directa
   de `vector<map<string,int>>`) e imprime los tiempos y `T_serial / T_paralelo`.

Correr ambas versiones en la misma ejecución, en la misma máquina y con el mismo caché de red, es
lo que hace que el speedup sea comparable: no depende de condiciones distintas entre dos corridas
separadas.

La parte paralela tiene tres fases:

**a) Descarga** — reparto estático por libro: el proceso `r` descarga los libros `i` tales que
`i mod q == r`. Funciona para cualquier k y q (incluyendo q > k: algunos procesos no descargan
nada). No reparte la red de forma pareja si los libros pesan distinto, pero la descarga pesa poco
frente al conteo (ver [evaluación experimental](evaluacion-experimental.md)).

**b) Conteo, repartido por trozos (no por libro)** — repartir el conteo por libro no balancea: un
libro puede pesar mucho más que los demás juntos (en `data/urls.txt`, `pg100` pesa más que los
otros 6 juntos), y el proceso que le toca ese libro tardaría tanto como la versión serial. Por eso
cada proceso corta **sus propios** libros en trozos de `TROZO = 256 * 1024` bytes (alargando el
corte hasta el siguiente byte que no sea alfanumérico, para no partir una palabra a la mitad) y
los reparte **por turnos** entre los q procesos con `MPI_Alltoallv`. Cada proceso cuenta los
trozos que recibe (sin importar de qué libro son ni de quién los recibió) y suma los conteos por
libro con un `map<int, map<string,int>>`. Como se explicó en la sección 2, contar por trozos y
sumar da el mismo resultado que contar el libro completo.

Se probó variar `TROZO` entre 16 KB y 512 KB (q = 2, 4, 6, sobre `data/urls_locales.txt`): el
speedup no cambió de forma apreciable (las diferencias quedan dentro del ruido de medición entre
repeticiones). Se dejó en 256 KB porque da resultados tan buenos como cualquier otro valor
probado, con mensajes de MPI de tamaño razonable (ni demasiados mensajes chicos con TROZO
pequeño, ni trozos tan grandes que un solo trozo desbalancee a un proceso).

**c) Unión** — cada proceso aplana sus conteos a bytes (`empaquetarLibro`: por cada libro,
`[índice][n_palabras]` y luego cada palabra terminada en `'\0'` seguida de su conteo) y se juntan
en el proceso 0 con `MPI_Gather` (tamaños) + `MPI_Gatherv` (datos). El proceso 0 desempaqueta,
**suma** los conteos de cada libro (un libro puede llegar partido entre varios procesos, porque
sus trozos se repartieron por turnos), arma el vocabulario y escribe el CSV. Esta parte es
estrictamente serial (nadie más puede ayudar: hay que juntar todo en un solo proceso antes de
escribir un solo archivo) — es el límite de Amdahl del programa, medido aparte (`p_union` en la
salida) para que quede claro cuánto pesa.

## 5. Manejo de errores: `MPI_Abort`, no `exit()`

`bolsa_comun.h` lo comparten un programa de un solo proceso (serial) y uno de q procesos (MPI). Un
error (archivo que no abre, URL con caracteres inválidos, descarga que falla) no puede terminar el
programa llamando directamente a `exit()`: en MPI, si un solo proceso se sale y los demás se
quedan esperando en la siguiente barrera (`MPI_Barrier`), el programa se cuelga en vez de
terminar con un error claro.

Por eso los 5 puntos de error del header pasan por un puntero a función, `terminarPorError`, que
por defecto es `exit()` (lo que necesita `bolsa_serial.cpp`, donde no hay MPI). Al principio de
`main()`, `bolsa_mpi.cpp` lo reasigna:

```cpp
terminarPorError = [](int codigo) {
    MPI_Abort(MPI_COMM_WORLD, codigo);
    exit(codigo);  // por si MPI_Abort regresa en esta implementacion de MPI
};
```

`MPI_Abort(MPI_COMM_WORLD, codigo)` le pide a MPI que termine **todos** los procesos del
comunicador, no solo el que detectó el error. Probado forzando un error real (un listado de URLs
inexistente) con `-np 4`: los 4 procesos detectan el error (cada uno lee el listado) y el job
termina limpio con código de salida 1, sin ningún proceso colgado.

## 6. Cómo se mide el tiempo

- **Serial** (`bolsa_serial.cpp`): `chrono::steady_clock`, dos intervalos — descarga (`t0` a `t1`)
  y procesamiento, que incluye contar y escribir el CSV (`t1` a `t2`).
- **MPI** (`bolsa_mpi.cpp`): `MPI_Wtime()`, que mide tiempo de pared y está pensado para
  cronometrar código MPI. Se ponen `MPI_Barrier` antes y después de cada fase (descarga, conteo,
  unión) para que la medición de una fase no incluya el tiempo en que algunos procesos ya
  terminaron y otros no.
- El **speedup reportado** es `T_serial / T_paralelo` usando el tiempo **total** (descarga +
  procesamiento) de cada versión, porque el enunciado dice que el programa descarga los libros
  dinámicamente — no solo cuenta archivos ya descargados. También se imprime el speedup de
  **solo el procesamiento** por separado, para distinguir cuánto viene de paralelizar el conteo
  (que sí se reparte, sección 4b) contra cuánto de paralelizar la descarga (que se reparte, pero
  pesa menos y depende de la red).

## 7. Decisiones de diseño, resumidas

Ver la tabla completa de "Decisiones abiertas" en el [README](../README.md). Las de este
documento, ya tomadas y justificadas en el código:

| Decisión | Por qué |
|---|---|
| Reparto de descarga por libro (`i mod q`) | Simple, funciona para cualquier k y q; la descarga pesa poco frente al conteo |
| Reparto de conteo por trozos, no por libro | Un libro dominante (como `pg100`) rompería el balance si se repartiera por libro entero |
| `TROZO = 256 * 1024` | Probado entre 16 KB y 512 KB sin diferencia apreciable en el speedup |
| Unión con `Gather`/`Gatherv` en el proceso 0 | Hay que escribir un solo CSV; es el límite de Amdahl, medido aparte |
| `MPI_Abort` en vez de `exit()` para errores | Un `exit()` de un solo proceso en MPI deja a los demás colgados en la siguiente barrera |
| Nombrar temporales con el índice global del libro | Evita que dos procesos MPI se pisen el mismo archivo temporal |
