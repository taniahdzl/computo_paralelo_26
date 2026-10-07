# Descripción del código y estrategia de paralelización

## Versión serial

### Estructura general
...

### Paso 1 — detección de puntos core (primer orden)
...

### Paso 2 — reclasificación de ruido épsilon-alcanzable (core de segundo orden)
...

## Versión paralela P1 (matriz indivisible)

### Qué se paraleliza y por qué
...

### Directivas OpenMP usadas
...

### Secciones críticas / condiciones de carrera consideradas
...

## Versión paralela P2 (dividida por k)

### Cómo se divide la matriz
...

### Cómo se unen los resultados manteniendo consistencia
<!-- Este es el punto que el profesor remarca en el PDF: "¿Cómo unir los resultados
     para mantener consistencia?" — un punto que es core en una partición pero está
     cerca de la frontera con otra partición puede necesitar reclasificarse al unir. -->
...

### Directivas OpenMP usadas
...

## Diferencias clave entre P1 y P2
...