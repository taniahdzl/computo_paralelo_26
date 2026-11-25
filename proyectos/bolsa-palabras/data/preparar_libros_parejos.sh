#!/bin/bash
# Prueba de sensibilidad (robustez): parte temas/pg100.txt en 6 "libros" de tamano PAREJO
# (~900 KB cada uno) y genera data/urls_parejos.txt con sus rutas.
#
# Por que: data/urls_locales.txt (preparar_libros_locales.sh) tiene un libro grande dominante
# (como pg100 en data/urls.txt, >75% del texto); con un libro grande el reparto por trozos
# balancea facil y el speedup sale alto. Este script arma el caso contrario: SIN libro
# dominante, para confirmar que el speedup no se cae cerca del minimo exigido (1.2x) cuando
# todos los libros pesan igual.
#
# Uso (desde proyectos/bolsa-palabras):  bash data/preparar_libros_parejos.sh
set -e
ORIGEN="../../temas/pg100.txt"
DESTINO="data/libros_parejos"
mkdir -p "$DESTINO"
N=$(wc -l < "$ORIGEN")
PARTES=6
TAM=$((N / PARTES))
: > data/urls_parejos.txt
for i in $(seq 1 $PARTES); do
  ini=$(( (i - 1) * TAM + 1 ))
  if [ "$i" -eq "$PARTES" ]; then fin=$N; else fin=$((i * TAM)); fi
  sed -n "${ini},${fin}p" "$ORIGEN" > "$DESTINO/libro$i.txt"
  echo "$DESTINO/libro$i.txt" >> data/urls_parejos.txt
done
echo "Listo: $PARTES libros parejos en $DESTINO y listado en data/urls_parejos.txt"
