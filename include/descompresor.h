#ifndef DESCOMPRESOR_H
#define DESCOMPRESOR_H

#include "estadisticas.h"
#include "formato.h"

/* ---------------------------------------------------------------
   PENDIENTE: este modulo todavia no esta implementado.
   --------------------------------------------------------------- */

/* Lee el arbol de Huffman escrito en preorden a partir de la posicion
   actual de 'entrada'. Devuelve la raiz, o NULL si hubo error.
   El llamador debe liberarla con liberarArbol(). */
struct nodo *leerArbol(FILE *entrada);

/* Extrae un archivo del paquete y lo escribe en rutaSalida.
   'entrada' debe ser el paquete ya abierto y 'meta' su entrada del indice.
   Si firmaCoincide no es NULL, deja ahi 1 si el MD5 del archivo
   reconstruido coincide con el guardado en el indice, 0 si no.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int extraerArchivo(FILE *entrada, const EntradaIndice *meta,
                   const char *rutaSalida, int *firmaCoincide);

/* Expande un paquete completo en el directorio indicado.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int descomprimirPaquete(const char *rutaPaquete, const char *dirSalida,
                        Estadisticas *est);

#endif
