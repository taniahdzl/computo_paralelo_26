# Evaluación experimental de desempeño

## Definición del experimento

- Parámetros variados: k (solo P2), número de puntos, número de hilos.
- 10 iteraciones por configuración, se reporta el promedio.
- Métrica: tiempo de ejecución (`omp_get_wtime()`) y speedup contra la versión serial.

## Equipo utilizado

| | |
|---|---|
| CPU | |
| Cores físicos / virtuales | |
| RAM | |
| Sistema operativo | |
| Compilador y versión | |
| Flags de compilación | |

## Resultados

<!-- Insertar gráficas de speedup aquí, por ejemplo una por versión paralela,
     con el eje X = número de puntos o tamaño de chunk/k, series por número de hilos -->

## Interpretación y análisis

### ¿Por qué una versión paralela fue mejor (o equivalente) a la otra?
...

## Uso responsable y ético de IA Generativa

<!-- Según los lineamientos IEEE y Elsevier referenciados en el PDF del proyecto.
     Documentar: qué se usó, para qué, y qué se validó/entendió manualmente. -->

- Herramienta(s) usada(s):
- Partes del proyecto donde se usó:
- Cómo se verificó que el código entregado se entiende al 100%: