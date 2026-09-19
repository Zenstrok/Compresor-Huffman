#ifndef COMPRESOR_H
#define COMPRESOR_H

#include <stddef.h>
#include <stdio.h>

#include "estadisticas.h"
#include "formato.h"
#include "huffman.h"

/* Empaqueta los datos en bits usando el diccionario y los escribe.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int escribirDatosComprimidos(FILE *salida, const struct diccionario *diccionario,
                             const unsigned char *datos, size_t tamano);

/* Comprime un archivo y lo agrega como un bloque mas al paquete que ya
   esta abierto en 'salida'. Llena 'entrada' con los metadatos del archivo
   (nombre, tamano original, MD5, offset y tamano del bloque) para que el
   llamador los agregue al indice.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int agregarArchivoAlPaquete(FILE *salida, const char *rutaOriginal,
                            EntradaIndice *entrada);

/* Comprime un archivo a un bloque en memoria, en vez de escribirlo
   directamente en el paquete. Deja el bloque en *buffer (que el llamador
   debe liberar con free) y su tamano en *tamano, y llena 'entrada' con los
   metadatos. El campo offsetBloque queda en 0: lo define quien escriba el
   bloque en el paquete.
   Lo usa la version paralela, donde varios procesos comprimen a la vez y
   no pueden compartir un mismo FILE *.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int comprimirArchivoABuffer(const char *rutaOriginal, char **buffer,
                            size_t *tamano, EntradaIndice *entrada);

/* Comprime todos los archivos .txt de un directorio en un unico paquete.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int comprimirDirectorio(const char *dirEntrada, const char *rutaPaquete,
                        Estadisticas *est);

#endif
