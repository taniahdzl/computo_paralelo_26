---
name: documentar-solucion
description: Escribe o actualiza un .md que documenta una solución de código del curso (proyecto DBSCAN, ejercicios de OpenMP, etc.) explicando cómo funciona y justificando cada decisión con las diapositivas (PDFs de temas/) y las notas (temas/*/NN-notas.md). Úsalo cuando se pida "documentar", "explicar", "justificar" o "escribir el .md" de un archivo .cpp o de una versión (serial, P1, P2), o al terminar una solución nueva.
---

# Documentar una solución del curso

Objetivo: que Tania y Tamara puedan **explicar cualquier línea** del código en la defensa en vivo
(NOTA 5 del proyecto: si no, −80%) y que el documento cuente para la calificación de "calidad y
presentación de los documentos" (0.8 pts). El .md no es un resumen: es una **guía de estudio y de
defensa** del código.

## 1. Reunir el contexto (antes de escribir)

1. **Lee el código completo** que se va a documentar, y la versión de la que parte (p. ej. el serial
   si documentas P1/P2), para poder explicar **qué cambió y por qué**.
2. **Lee las notas relevantes** en `temas/`:
   - `01-introduccion-arquitectura/01-notas.md`: speedup, Amdahl, memoria, caché, condiciones de carrera.
   - `02-open-mp/02-notas.md`: fork-join, `parallel for`, `schedule`, fors anidados, `critical`,
     `reduction`, cómo compilar en la Mac.
   - `03-dbscan/03-notas.md`: algoritmo de 2 pasos, ideas de P1/P2, experimento, rúbrica.
3. **Abre las diapositivas** cuando una justificación dependa de ellas. Usa la herramienta Read
   **sin** el parámetro `pages` (con `pages` pide poppler, que no está instalado). Las notas ya
   citan páginas como `(p33)`; verifica la página en el PDF antes de citarla de nuevo.
4. **Corre el código** si vas a afirmar algo medible (tiempo, speedup, que da las mismas etiquetas).
   No escribas números que no hayas medido; si no hay medición, dilo ("pendiente de medir").

## 2. Dónde va el .md

| Qué se documenta | Archivo |
|---|---|
| Serial, P1, P2 del proyecto DBSCAN | `proyectos/dbscan/docs/code-description.md` (una sección por versión; es un entregable) |
| Resultados del experimento | `proyectos/dbscan/docs/experimental-eval.md` (otro entregable; no mezclar) |
| Un ejercicio de `ejercicios/` | `ejercicios/<nombre>.md`, junto al `.cpp` |

Si el archivo ya existe, **actualiza la sección** correspondiente; no dupliques ni borres lo que
escribieron ellas a mano. Después, agrega o actualiza el enlace en la tabla del `README.md` raíz
("Ejercicios de clase" o "Proyectos") y marca la tarea en "Pendientes" si ya quedó hecha.

## 3. Estructura de cada sección (por solución)

Usa esta plantilla. Los títulos pueden ajustarse, pero no omitas ninguna parte:

```markdown
## <Nombre de la versión> — `<ruta/al/archivo.cpp>`

### Idea en una frase
<Qué hace y cuál es la estrategia, sin jerga innecesaria.>

### Cómo se compila y se corre
<Comando exacto para la Mac (clang++ -Xpreprocessor -fopenmp ...) y un ejemplo con argumentos.>

### Recorrido del código, bloque por bloque
<Por cada bloque (lectura, inicialización, cronómetro, paso 1, paso 2, escritura, liberar memoria):
 - líneas que abarca (`archivo.cpp:12-30`),
 - qué hace,
 - por qué está escrito así.
 Si es una versión paralela, marca qué es igual al serial y qué es nuevo.>

### Decisiones de paralelización y su justificación
| Decisión | Alternativa descartada | Por qué | Fuente |
|---|---|---|---|
| `schedule(static)` en el Paso 1 | `dynamic` | todas las iteraciones cuestan lo mismo | nota 02, (p40–p44) |

### Por qué es correcto (sin condiciones de carrera)
<Qué variables son compartidas y cuáles privadas; quién escribe qué posición; dónde hay barreras y
por qué se necesitan; cómo se validó (p. ej. "0 etiquetas distintas vs serial con 4k, 20k, 80k").>

### Costo y rendimiento esperado
<Complejidad, qué limita (cómputo vs memoria), speedup medido si existe, y por qué.>

### Preguntas que nos pueden hacer en la defensa
1. <Pregunta> — <respuesta corta, en 1–3 líneas>.
```

## 4. Reglas de estilo

- **En español**, con el tono de las notas de `temas/`: directo, frases cortas, tablas cuando se
  comparan opciones y **negritas** solo para lo que hay que memorizar.
- **Cita la fuente de cada justificación**: diapositiva `(p33)` + nota (`nota 02`), o "medido en esta
  Mac" con el número. Si una decisión no viene de clase (p. ej. copiar los candidatos a arreglos
  contiguos), dilo explícitamente y explica la razón técnica.
- **Referencias al código** como `ruta/archivo.cpp:línea` para que se puedan abrir con un clic.
  Revisa los números de línea contra el archivo actual.
- Explica los términos la primera vez que aparecen (barrera implícita, variable privada, halo…).
- No pegues el archivo completo; cita fragmentos cortos solo donde ayuden a explicar.
- Nada inventado: si algo no se pudo verificar, márcalo como `> ⚠️ Pendiente:`.

## 5. Antes de terminar

- [ ] Cada directiva `#pragma omp` del código está explicada y justificada.
- [ ] Cada variable compartida tiene explicado por qué no causa una condición de carrera.
- [ ] Las citas `(pNN)` existen en el PDF y los `archivo:línea` coinciden con el código actual.
- [ ] Los números (tiempos, speedups, conteos de ruido) vienen de corridas reales.
- [ ] El `README.md` raíz enlaza el documento y los pendientes están actualizados.
- [ ] Hay al menos 5 preguntas de defensa con respuesta.
