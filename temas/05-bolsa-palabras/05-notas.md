# 05. Proyecto Clausura: Bolsa de palabras con MPI

> Fuente: `Proyecto_Clausura_2026a.pdf` (7 diapositivas) y la libreta de Colab
> `computo_paralelo_bolsa_de_palabras.ipynb`. Lo que no viene en esas fuentes está marcado
> como **suposición** o **por confirmar**.

## 1. Qué pide el enunciado

- Contar palabras de **k libros** de Project Gutenberg y producir su **matriz de bolsa de palabras**.
- Entrada: número de procesos y un listado de URLs de Project Gutenberg. Los libros se descargan
  dinámicamente a partir de las URLs.
- Salida: archivo CSV con la matriz.
- Implementar **versión serial y versión paralela con MPI**.
- Calcular e imprimir el **speedup** (tiempo de la versión paralela contra el de la serial).
- Debe funcionar para **k libros y q procesos**.
- Evaluación: speedup **mayor a 1.2x**; 2.0 puntos de la calificación final (1.9 por ejecución con
  todos los requerimientos y 0.1 por código documentado).
- Descuentos: **-1 punto** si el paralelo no es más eficiente que el serial; **-30% por día natural
  de retraso**; entregar después de la hora de clase cuenta como un día más.
- Entrega: **martes 1 de diciembre de 2026**, en clase, equipo de 1 o 2 personas.
- La diapositiva final muestra un ejemplo de diccionario `std::map<string, int>` con `insert`,
  `at`, `[]`, `count`, `erase` y recorrido con iterador.

## 2. Qué es una bolsa de palabras

Se ignora el orden de las palabras y solo se cuentan cuántas veces aparece cada una. Con k
documentos y un vocabulario de V palabras diferentes, la matriz tiene **k filas y V columnas**:
la celda (i, j) es el número de veces que la palabra j aparece en el libro i.

Ejemplo de la libreta con 6 libros: forma **6 x 15,170**.

## 3. Qué hace la libreta de Colab

1. Lee 6 archivos `libro1.txt` a `libro6.txt`. Cada archivo ya viene con las palabras separadas por
   comas (por ejemplo `project,gutenberg,ebook,christmas,carol,charles,...`).
2. Usa `CountVectorizer(min_df=0.0, max_df=1.0)` de sklearn para construir la matriz.
3. Muestra la matriz como un DataFrame con **una fila por libro y una columna por palabra**; el
   vocabulario sale **en orden alfabético** (los números como `000` van antes que las letras).
4. Guarda el vocabulario en `vocab.txt`.

Qué hace `CountVectorizer` con sus valores por defecto (de sklearn, no del enunciado):

- Pasa todo a **minúsculas**.
- Una palabra es una secuencia de **2 o más** letras o números; las de un solo carácter no se cuentan.
- No quita palabras vacías a menos que se le pida.

**Observaciones sobre la libreta** (suposiciones mías, hay que confirmarlas):

- Los primeros términos del libro 1 sugieren que se quitaron palabras como "the", "of", "a" y "by"
  antes de contar.
- El encabezado de Gutenberg sí se cuenta (aparecen `project`, `gutenberg`, `ebook`).
- Aparecen las columnas `zuno`, `zdos`, `ztres` y `zseis` con conteos de 1 a 6; parecen palabras
  de prueba que se insertaron para verificar la matriz.
- Las salidas muestran 15,170 columnas en una celda y 15,164 palabras en otra: las celdas se
  corrieron en momentos distintos. No usar esos números como valores de referencia exactos.

## 4. Qué falta definir

Ver la tabla de decisiones abiertas en [`proyectos/bolsa-palabras/README.md`](../../proyectos/bolsa-palabras/README.md).
Las más importantes: limpieza del texto y palabras vacías (cambian el resultado), y qué entra en el
tiempo medido (cambia el speedup).

## 5. Ideas para la versión MPI (a evaluar, no decididas)

- **Dónde descargar:** cada proceso descarga sus propios libros (reparte el tiempo de red y evita
  mandar textos grandes por MPI) o el proceso 0 descarga todo y lo reparte.
- **Reparto de libros:** estático (libro i al proceso i mod q) o dinámico (maestro y trabajadores con
  `MPI_Send` y `MPI_Recv`). Los libros pesan distinto y puede haber más procesos que libros.
- **Juntar resultados:** cada proceso cuenta sus libros con un `map<string, int>` y manda sus pares
  palabra-conteo al proceso 0. Mandar cadenas con MPI requiere aplanarlas (por ejemplo, un solo
  texto con separadores más un arreglo de enteros).
- **Escribir el CSV:** lo hace el proceso 0. Es trabajo serial y limita el speedup (Amdahl).
- **Riesgo:** contar palabras en pocos libros tarda poco, y los costos de MPI y de escribir el CSV
  pueden comerse el speedup. Hay que medir temprano. (No está medido.)

## 6. Conexiones con lo visto en clase

- MPI: rank, size, `MPI_Send` y `MPI_Recv`, `MPI_Bcast`, `MPI_Scatter`, `MPI_Gather`, `MPI_Reduce`,
  `MPI_Wtime` solo en el proceso 0 (notas 04).
- Speedup y Amdahl (notas 01).
- La práctica de CUDA usa también obras de Gutenberg (histograma de caracteres de Shakespeare).
