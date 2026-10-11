# Cómputo Paralelo y en la Nube — ITAM

Apuntes, guías de estudio y proyectos del curso. Repositorio personal para repasar antes del examen (incluye evaluación en vivo con dataset dado por el profesor).

## Temas vistos

| # | Tema | Notas | Slides |
|---|------|-------|--------|
| 01 | Introducción y arquitecturas paralelas (speedup, Amdahl, Flynn, UMA/NUMA, memoria) | [01-notas.md](temas/01-introduccion-arquitectura/01-notas.md) | [PDF](<temas/01-introduccion-arquitectura/Introduccion y arquitecturas paralelas.pdf>) |
| 02 | OpenMP (C++ básico, fork-join, parallel for, schedule, critical, reduction) | [02-notas.md](temas/02-open-mp/02-notas.md) | [OpenMP.pdf](temas/02-open-mp/OpenMP.pdf) |
| 03 | DBSCAN paralelo (Proyecto Apertura) | [03-notas.md](temas/03-dbscan/03-notas.md) | [PDF](temas/03-dbscan/ProyectoApertura2026bDBSCAN.pdf) |

> Actualiza esta tabla cada vez que agregues un tema nuevo.

## Ejercicios de clase

| Ejercicio | Descripción | Código |
|---|---|---|
| Suma de vectores con OpenMP | `#pragma omp parallel for` sobre dos fors independientes | [suma_vectores.cpp](ejercicios/suma_vectores.cpp) |

## Proyectos

- [**Proyecto Apertura — DBSCAN paralelo**](proyectos/dbscan/README.md): detección de ruido/outliers con OpenMP, serial + 2 versiones paralelas. Entrega: 20 de octubre de 2026.

## Pendientes

Hecho: setup de VS Code + `.venv`, `libomp` instalado, versión serial validada contra sklearn
(4000 puntos, 0 diferencias) y notas de los temas 01–03.

**Ahora**
- [ ] Leer las notas de `temas/` (01 → 02 → 03) y anotar dudas en "Dudas resueltas" de cada una.

**Ejercicios de calentamiento** (te pueden pedir hacerlos en vivo)
- [ ] `ejercicios/suma_vectores.cpp`: suma de vectores con `#pragma omp parallel for` (nota 02).
- [ ] `ejercicios/parallel_fors.cpp`: `parallel for`, `schedule`, `collapse` y los errores comunes (nota 02).

**Proyecto DBSCAN** (entrega: martes 20 de octubre de 2026, en clase)
- [ ] Decidir si se compara `distancia² <= eps²` (sin `sqrt`). Si se cambia, debe ser igual en las 3 versiones.
- [ ] P1 (`src/parallel/dbscan_p1.cpp`): `parallel for` en los dos pasos; probar `static` vs `dynamic`.
- [ ] P2 (`src/parallel-2/dbscan_p2.cpp`): decidir cómo dividir en `k` (propuesta: cuadrícula del
      plano + franja "halo" de ancho eps para no perder vecinos en las fronteras).
- [ ] Validar que P1 y P2 dan exactamente las mismas etiquetas que el serial.
- [ ] Script que corra todo el experimento (k × puntos × hilos {1, 8, 16, 32} × 10 repeticiones) y
      guarde `experiments/resultados.csv`. Dejarlo corriendo con tiempo: son ~2,300 corridas.
- [ ] Gráficas de speedup (P1 y P2 vs serial). Al menos una versión debe pasar de **1.5x**.
- [ ] `docs/code-description.md`: explicación del código y de las estrategias de paralelización.
- [ ] `docs/experimental-eval.md`: experimento, hardware/software, análisis (¿por qué P1 o P2 fue
      mejor?), uso responsable de IA generativa.
- [ ] Arreglar `proyectos/dbscan/README.md`: nombres de carpetas/archivos (`parallel/`, `parallel-2/`,
      `docs/code-description.md`, `experiments/`), comando de ejemplo con `eps` y `min_samples`,
      comandos de compilación para Mac, y nombres del equipo.
- [ ] Practicar explicar cada línea de serial, P1 y P2 (si no se puede explicar, −80%).

> Código de prácticas del curso: https://github.com/octavio-gutierrez/computoparalelo2026b

## Cómo usar este repo para repasar

1. Antes del examen, recorre `temas/` en orden y lee solo las secciones de "Conceptos clave" de cada `notas.md`.
2. Si algo no te queda claro, revisa el slide original junto a la nota.
3. Para la parte práctica (exigen correr código en vivo), practica compilando y corriendo los ejercicios de `ejercicios/` desde cero, sin copiar y pegar.