#!/bin/bash
# Parte temas/pg100.txt (obras completas de Shakespeare) en 6 "libros" de tamanos distintos y
# genera data/urls_locales.txt con sus rutas. Sirve para probar y medir SIN depender de
# gutenberg.org (el programa acepta rutas locales en lugar de URLs).
# Uso (desde proyectos/bolsa-palabras):  bash data/preparar_libros_locales.sh
set -e
ORIGEN="../../temas/pg100.txt"
DESTINO="data/libros_prueba"
mkdir -p "$DESTINO"
N=$(wc -l < "$ORIGEN")
: > data/urls_locales.txt
i=0
for rango in "1 3000" "3001 4000" "4001 9000" "9001 12000" "12001 30000" "30001 $N"; do
  i=$((i+1)); set -- $rango
  sed -n "$1,$2p" "$ORIGEN" > "$DESTINO/libro$i.txt"
  echo "$DESTINO/libro$i.txt" >> data/urls_locales.txt
done
echo "Listo: $i libros en $DESTINO y listado en data/urls_locales.txt"
