#ifndef COMPRESOR_H
#define COMPRESOR_H

#include <stddef.h>
#include <stdio.h>

#include "huffman.h"

/* Empaqueta los datos en bits usando el diccionario y los escribe.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int escribirDatosComprimidos(FILE *salida, const struct diccionario *diccionario,
                             const unsigned char *datos, size_t tamano);

/* Comprime un archivo completo: calcula su MD5, construye el arbol,
   escribe los metadatos y el flujo de bits en rutaComprimida.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int comprimirArchivo(const char *rutaOriginal, const char *rutaComprimida);

#endif
