#ifndef DIRECTORIO_H
#define DIRECTORIO_H

/* Lista de rutas de archivos encontrados dentro de un directorio. */
typedef struct {
    char **rutas;       /* arreglo de rutas completas */
    int cantidad;
} ListaArchivos;

/* Recorre el directorio y llena la lista con los archivos regulares que
   terminan en 'extension' (por ejemplo ".txt"). Si extension es NULL,
   incluye todos los archivos regulares. No entra en subdirectorios.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int listarArchivos(const char *rutaDirectorio, const char *extension,
                   ListaArchivos *lista);

void liberarListaArchivos(ListaArchivos *lista);

/* Crea el directorio si no existe. Devuelve 1 si existe o se creo. */
int asegurarDirectorio(const char *ruta);

/* Construye 'destino' combinando el directorio, el nombre base de 'rutaArchivo'
   y la extension nueva. Ejemplo:
   ("salida", "libros/quijote.txt", ".huff") -> "salida/quijote.huff"
   Devuelve 1 si cupo en el buffer, 0 si no. */
int rutaDestino(char *destino, size_t tamanoDestino, const char *directorio,
                const char *rutaArchivo, const char *extensionNueva);

#endif
