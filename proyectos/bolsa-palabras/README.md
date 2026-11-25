# Proyecto Clausura: Bolsa de palabras con MPI

Curso: Cómputo Paralelo y en la Nube, ITAM. Profesor: Dr. J. Octavio Gutiérrez García.
Equipo: Tania Hernandez y Tamara Schnaas.
**Entrega: martes 1 de diciembre de 2026, en clase.**

> Fuente del enunciado: `Proyecto_Clausura_2026a.pdf` (7 diapositivas) y la libreta de Colab
> `computo_paralelo_bolsa_de_palabras.ipynb`. Las notas están en
> [`temas/05-bolsa-palabras/05-notas.md`](../../temas/05-bolsa-palabras/05-notas.md).

## Qué hay que hacer

Dado un conjunto de k libros de Project Gutenberg, construir la **matriz de bolsa de palabras**:
una fila por libro, una columna por palabra del vocabulario, y en cada celda cuántas veces aparece
la palabra en el libro.

| | Detalle |
|---|---|
| Entrada | Número de procesos y un listado de URLs de Project Gutenberg (los libros se descargan dinámicamente) |
| Salida | Archivo CSV con la matriz de bolsa de palabras |
| Versiones | Una serial y una paralela con MPI |
| Debe | Funcionar para k libros y q procesos; calcular e imprimir el speedup (tiempo serial contra tiempo paralelo) |

## Criterios de evaluación (2.0 puntos de la calificación final)

- Speedup **mayor a 1.2x**.
- Ejecución con todos los requerimientos: **1.9 puntos**.
- Código documentado correctamente: **0.1 puntos**.
- **-1 punto** si el código paralelo no es más eficiente que el serial.
- **-30% por cada día natural de retraso**. Entregar después de la hora de clase cuenta como un día más.

## Decisiones abiertas (confirmar con el profesor o decidir y justificar)

El enunciado no las define. Mientras no se confirmen, esto es lo que se asume:

| Decisión | Se asume | Estado |
|---|---|---|
| Orientación y formato del CSV | Encabezado con las palabras, una fila por libro (igual que la libreta) | Por confirmar |
| Limpieza del texto | Minúsculas; palabra = 2 o más caracteres `a-z` o `0-9` (**solo ASCII**). `_` y letras con acento separan palabras. `validar_sklearn.py` usa el mismo patrón (`token_pattern`); con el patrón por defecto de sklearn la comparación nunca coincidiría, porque Gutenberg usa `_cursivas_` | Decidido (documentar) |
| Palabras vacías (stopwords) | La libreta parece haberlas quitado antes de contar; el enunciado no las menciona | Por confirmar |
| Encabezado y pie de Gutenberg | Se cuentan (en la libreta sí aparecen `project`, `gutenberg`, `ebook`) | Por confirmar |
| Qué entra en el tiempo medido | Propuesta: métrica principal = total **con** descarga (el enunciado dice que el programa descarga los libros); se imprime también el tiempo solo de procesamiento. Solo el procesamiento (~0.1 s con 6 libros) deja muy poco margen para 1.2x por Amdahl (el CSV lo escribe el rank 0) | Por confirmar |
| Cómo se imprime el speedup | Propuesta: `bolsa_mpi` corre en una sola ejecución la parte serial (rank 0) y luego la paralela, e imprime `T_serial`, `T_paralelo` y `T_serial / T_paralelo` | Por decidir |
| Cómo descargar | `curl` desde el programa (`std::system`), o libcurl | Por decidir |

## Estructura

```
proyectos/bolsa-palabras/
├── README.md                          este archivo
├── docs/
│   ├── descripcion-codigo.md          cómo funciona el código y por qué (diseño, MPI_Abort, etc.)
│   ├── evaluacion-experimental.md     metodología, resultados y gráfica del speedup
│   └── robustez-sin-libro-dominante.md  prueba aparte: speedup sin un libro grande en el corpus
├── src/
│   ├── comun/bolsa_comun.h            funciones compartidas (descarga, conteo, vocabulario, CSV)
│   ├── serial/bolsa_serial.cpp
│   └── mpi/bolsa_mpi.cpp              serial + paralelo en una corrida, imprime el speedup
├── data/
│   ├── urls.txt                       listado de URLs de prueba
│   ├── preparar_libros_locales.sh     corpus local con un libro dominante (sin red)
│   ├── preparar_libros_parejos.sh     corpus local sin libro dominante (sin red)
│   └── validar_sklearn.py             genera la matriz esperada con CountVectorizer para comparar
└── experiments/
    ├── corridas_locales.txt, resultados_locales.csv, speedup_locales.png   experimento principal
    ├── corridas_parejo.txt, resultados_parejo.csv, speedup_parejo.png      prueba de robustez
    └── analizar_corridas.py           resume un archivo de corridas en un CSV y una gráfica
```

## Cómo compilar y correr

```
# serial
g++ -O2 -std=c++17 src/serial/bolsa_serial.cpp -o bolsa_serial
./bolsa_serial data/urls.txt salida_serial.csv

# MPI (Mac: brew install open-mpi; Ubuntu/WSL: sudo apt install openmpi-bin libopenmpi-dev)
mpic++ -O2 -std=c++17 src/mpi/bolsa_mpi.cpp -o bolsa_mpi
mpirun -np 4 ./bolsa_mpi data/urls.txt salida_mpi.csv   # tambien escribe salida_mpi.csv.serial.csv
```

## Notas para la versión MPI

- Los archivos temporales de descarga se nombran con el **índice global** del libro
  (`libro_<i>.txt`); si cada proceso usara su propio contador, dos procesos podrían escribir el mismo archivo.
- Descarga: reparto por libro (`i mod q`). Conteo: reparto por **trozos de texto** (256 KB, cortados en
  un byte separador). Repartir el conteo por libro **no** balancea: un libro puede ser el 75% del texto
  (`pg100` pesa más de 5 MB; los otros seis suman menos de 3 MB), y si le tocara entero a un solo
  proceso, ese proceso tardaría casi lo mismo que la versión serial completa — limitaría el speedup a
  ~1.2x (el mínimo exigido, apenas). Con trozos repartidos por turnos (`MPI_Alltoallv`) el conteo
  queda parejo entre todos los procesos sin importar cómo esté repartido el texto entre libros; medido
  así, el speedup real llega a 1.6x–2.6x (ver `docs/evaluacion-experimental.md`). Contar por trozos y
  sumar da exactamente lo mismo que contar el libro completo.
- La unión en el proceso 0 (sumar, vocabulario, CSV) es serial: ~0.05 s en las corridas medidas, sin
  crecer con `q`. Es el límite de Amdahl del programa.

## Checklist

- [ ] Confirmar con el profesor las decisiones abiertas de arriba
- [x] Versión serial validada contra `validar_sklearn.py` con archivos locales (falta con descarga real)
- [x] Comparar el CSV serial contra `data/validar_sklearn.py` (`bolsa_serial` solo, aparte de `bolsa_mpi`: idénticas, 23,655 palabras)
- [x] Versión MPI funcionando para k libros y q procesos (probada con q = 1, 2, 3, 4, 6, 8 y 11 sobre 6 libros; q = 11 es el máximo de núcleos físicos en la máquina de prueba)
- [x] Comparar el CSV de MPI contra el serial (`bolsa_mpi` lo hace al final de cada corrida; 35/35 corridas idénticas en el experimento de abajo)
- [x] Imprimir el tiempo serial, el tiempo paralelo y el speedup
- [x] Speedup mayor a 1.2x en varias corridas seguidas — **con libros locales** (sin descarga real): media 1.6x–2.6x para q ≥ 2, peor repetición de 30 corridas = 1.44x. Ver [`docs/evaluacion-experimental.md`](docs/evaluacion-experimental.md) y, para el caso sin un libro dominante en el corpus, [`docs/robustez-sin-libro-dominante.md`](docs/robustez-sin-libro-dominante.md). Falta confirmar con descarga real (bloqueada por mantenimiento de Gutenberg, ver abajo)
- [x] Código documentado (comentarios que explican cada bloque y las decisiones de diseño, en `src/` y ampliado en `docs/descripcion-codigo.md`)
- [x] Descripción del código y evaluación experimental en `docs/` (`descripcion-codigo.md`, `evaluacion-experimental.md`, `robustez-sin-libro-dominante.md`); falta la sección de descarga real una vez que Gutenberg esté disponible
- [ ] Verificar que las 7 URLs de `data/urls.txt` abren el libro correcto (bloqueado: Gutenberg está en mantenimiento — 503 "Temporarily Down for Maintenance" la última vez que se intentó)
- [ ] Una corrida con descarga real desde Gutenberg, documentada en `docs/evaluacion-experimental.md` y guardada en `experiments/corridas_red.txt`
- [x] `MPI_Abort` en vez de `exit()` para los errores dentro de la versión MPI (probado forzando un error real con `-np 4`: ver `docs/descripcion-codigo.md`, sección 5)
- [ ] Entrega el martes 1 de diciembre de 2026, antes de la clase

## Calendario sugerido

| Fechas | Qué |
|---|---|
| 17 a 20 de noviembre | Confirmar decisiones; versión serial; validar contra la libreta |
| 21 a 25 de noviembre | Versión MPI; comparar CSV |
| 26 a 28 de noviembre | Medir speedup; ajustar; documentar el código |
| 29 y 30 de noviembre | Margen de seguridad y ensayo de la entrega |
| 1 de diciembre | Entrega |
