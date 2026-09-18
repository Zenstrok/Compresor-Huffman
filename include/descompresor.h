#ifndef DESCOMPRESOR_H
#define DESCOMPRESOR_H

#include <stdint.h>
#include <stdio.h>

#include "huffman.h"
#include "md5.h"

/* ---------------------------------------------------------------
   PENDIENTE: este modulo todavia no esta implementado.
   Es la parte que corresponde a Mario.
   --------------------------------------------------------------- */

/* Descomprime un .huff y escribe el resultado en rutaSalida.
   Si firmaCoincide no es NULL, deja ahi 1 si el MD5 del archivo
   reconstruido coincide con el guardado en los metadatos, 0 si no.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int descomprimirArchivo(const char *rutaComprimida, const char *rutaSalida,
                        int *firmaCoincide);

#endif
