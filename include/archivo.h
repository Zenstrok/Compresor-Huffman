#ifndef ARCHIVO_H
#define ARCHIVO_H

#include <stddef.h>

/* Carga el archivo completo en memoria. Deja el buffer en *datos (que el
   llamador debe liberar con free) y el tamano en *tamano.
   Si el archivo esta vacio deja *datos en NULL y *tamano en 0.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int cargarArchivo(const char *ruta, unsigned char **datos, size_t *tamano);

#endif
