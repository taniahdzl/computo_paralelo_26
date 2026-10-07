# Cómputo Paralelo y en la Nube — ITAM

Apuntes, guías de estudio y proyectos del curso. Repositorio personal para repasar antes del examen (incluye evaluación en vivo con dataset dado por el profesor).

## Temas vistos

| # | Tema | Notas | Slides |
|---|------|-------|--------|
| 01 | Tipos de lenguajes (compilado, interpretado, JIT) | [notas.md](temas/01-tipos-de-lenguajes/notas.md) | — |
| 02 | Arquitectura: cores, threads, procesos | [notas.md](temas/02-arquitectura-cpu/notas.md) | — |
| 03 | OpenMP — fundamentos | [notas.md](temas/03-openmp-fundamentos/notas.md) | [OpenMP.pdf](temas/03-openmp-fundamentos/slides.pdf) |
| 04 | DBSCAN paralelo | [notas.md](temas/04-dbscan/notas.md) | — |

> Actualiza esta tabla cada vez que agregues un tema nuevo.

## Ejercicios de clase

| Ejercicio | Descripción | Código |
|---|---|---|
| Suma de vectores con OpenMP | `#pragma omp parallel for` sobre dos fors independientes | [suma_vectores.cpp](ejercicios/01-suma-vectores-openmp/suma_vectores.cpp) |

## Proyectos

- [**Proyecto Apertura — DBSCAN paralelo**](proyectos/apertura-dbscan/README.md): detección de ruido/outliers con OpenMP, serial + 2 versiones paralelas. Entrega: 20 de octubre de 2026.

## Cómo usar este repo para repasar

1. Antes del examen, recorre `temas/` en orden y lee solo las secciones de "Conceptos clave" de cada `notas.md`.
2. Si algo no te queda claro, revisa el slide original junto a la nota.
3. Para la parte práctica (exigen correr código en vivo), practica compilando y corriendo los ejercicios de `ejercicios/` desde cero, sin copiar y pegar.