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
├── README.md                  este archivo
├── docs/                      descripción del código y evaluación experimental
├── src/
│   ├── comun/bolsa_comun.h    funciones compartidas (descarga, conteo, vocabulario, CSV)
│   ├── serial/bolsa_serial.cpp
│   └── mpi/bolsa_mpi.cpp      serial + paralelo en una corrida, imprime el speedup
├── data/
│   ├── urls.txt               listado de URLs de prueba
│   └── validar_sklearn.py     genera la matriz esperada con CountVectorizer para comparar
└── experiments/               tiempos y gráficas
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
- Repartir con `i mod q` sirve para empezar, pero los libros pesan muy distinto (uno solo puede ser
  ~40% del conteo). Medir antes de pasar a un esquema maestro-trabajador.

## Checklist

- [ ] Confirmar con el profesor las decisiones abiertas de arriba
- [x] Versión serial validada contra `validar_sklearn.py` con archivos locales (falta con descarga real)
- [ ] Comparar el CSV serial contra `data/validar_sklearn.py`
- [x] Versión MPI funcionando para k libros y q procesos (probada con q = 1, 2, 3, 4 y 8 sobre 6 libros)
- [x] Comparar el CSV de MPI contra el serial (`bolsa_mpi` lo hace al final de cada corrida)
- [x] Imprimir el tiempo serial, el tiempo paralelo y el speedup
- [ ] Speedup mayor a 1.2x en varias corridas seguidas
- [ ] Código documentado (comentarios que expliquen cada bloque)
- [ ] Descripción del código y evaluación experimental en `docs/`
- [ ] Entrega el martes 1 de diciembre de 2026, antes de la clase

## Calendario sugerido

| Fechas | Qué |
|---|---|
| 17 a 20 de noviembre | Confirmar decisiones; versión serial; validar contra la libreta |
| 21 a 25 de noviembre | Versión MPI; comparar CSV |
| 26 a 28 de noviembre | Medir speedup; ajustar; documentar el código |
| 29 y 30 de noviembre | Margen de seguridad y ensayo de la entrega |
| 1 de diciembre | Entrega |
