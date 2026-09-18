#ifndef FORMATO_H
#define FORMATO_H

#include <stdint.h>
#include <stdio.h>

#include "huffman.h"
#include "md5.h"

/* ---------------------------------------------------------------
   Formato del archivo .huff (version HUFFV1)

   offset  bytes     contenido
   ------  --------  ------------------------------------------------
        0         6  identificador "HUFFV1"
        6        16  MD5 del archivo original, crudo (no hexadecimal)
       22         8  uint64_t con el tamano original en bytes
       30         1  tieneArbol: 1 si hay arbol, 0 si el archivo estaba vacio
       31  variable  arbol en preorden (solo si tieneArbol == 1)
      ...     resto  flujo de bits

   Arbol en preorden:
     0x00                -> nodo interno; siguen su hijo izquierdo y el derecho
     0x01 + 1 byte       -> hoja con ese caracter

   Flujo de bits:
     del bit mas significativo al menos significativo dentro de cada byte.
     Izquierda = 0, derecha = 1. El ultimo byte se rellena con ceros a la
     derecha, por lo que hay que detener la decodificacion al llegar a
     tamanoOriginal caracteres y no al agotar el archivo.
   --------------------------------------------------------------- */

#define MAGIC_SIZE 6

extern const unsigned char MAGIC_HUFF[MAGIC_SIZE];

/* Escribe el arbol en preorden. Devuelve 1 si tuvo exito, 0 si hubo error. */
int escribirArbol(FILE *salida, struct nodo *raiz);

/* Escribe el encabezado completo (magic, MD5, tamano, arbol).
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int escribirMetadatos(FILE *archivoComprimido, const unsigned char firma[MD5_SIZE],
                      uint64_t tamanoOriginal, struct nodo *raiz);

#endif
