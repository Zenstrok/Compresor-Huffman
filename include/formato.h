#ifndef FORMATO_H
#define FORMATO_H

#include <stdint.h>
#include <stdio.h>

#include "huffman.h"
#include "md5.h"

/* ---------------------------------------------------------------
   Formato del paquete comprimido (.huff), version HUFFPKG1

   Un solo archivo .huff contiene TODOS los archivos del directorio.

   ENCABEZADO (20 bytes, al inicio del archivo)
     offset  bytes  contenido
     ------  -----  --------------------------------------------
          0      8  identificador "HUFFPKG1"
          8      4  uint32: cantidad de archivos en el paquete
         12      8  uint64: offset donde empieza el indice

   BLOQUES (desde el offset 20, uno detras de otro, uno por archivo)
          -      1  tieneArbol: 1 si hay arbol, 0 si el archivo
                    original estaba vacio
          -    var  arbol de Huffman en preorden
          -    var  flujo de bits del archivo

   INDICE (al final, en el offset que indica el encabezado)
     Una entrada por archivo, en el mismo orden que los bloques:
          -      2  uint16: largo del nombre, sin terminador nulo
          -    var  nombre del archivo original
          -      8  uint64: tamano original en bytes
          -     16  MD5 del archivo original, crudo (no hexadecimal)
          -      8  uint64: offset donde empieza su bloque
          -      8  uint64: tamano del bloque en bytes

   ARBOL EN PREORDEN
     0x00           -> nodo interno; siguen su hijo izquierdo y el derecho
     0x01 + 1 byte  -> hoja con ese caracter

   FLUJO DE BITS
     Del bit mas significativo al menos significativo dentro de cada byte.
     Izquierda = 0, derecha = 1. El ultimo byte de cada bloque se rellena
     con ceros a la derecha, por lo que hay que detener la decodificacion
     al llegar a tamanoOriginal caracteres y no al agotar el bloque.

   NOTAS DE DISENO
     - El indice va al final para que el compresor pueda escribir los
       bloques de corrido y anotar los offsets sobre la marcha.
     - Cada archivo conserva su propio arbol: comprime mejor que un arbol
       global y no obliga a leer todo el directorio antes de empezar.
     - El offset de cada bloque permite que las versiones con fork() y
       pthread() salten directo al archivo que les toca, sin leer el
       paquete desde el principio.
     - Todos los enteros se escriben en little-endian de forma explicita,
       para que el formato no dependa de la arquitectura.
   --------------------------------------------------------------- */

#define MAGIC_SIZE      8
#define TAMANO_ENCABEZADO 20
#define MAX_NOMBRE      256

extern const unsigned char MAGIC_PAQUETE[MAGIC_SIZE];

/* Metadatos de un archivo dentro del paquete. */
typedef struct {
    char nombre[MAX_NOMBRE];
    uint64_t tamanoOriginal;
    unsigned char firma[MD5_SIZE];
    uint64_t offsetBloque;
    uint64_t tamanoBloque;
} EntradaIndice;

/* Indice completo del paquete. */
typedef struct {
    EntradaIndice *entradas;
    uint32_t cantidad;
} Indice;

/* ---- Escritura (compresor) ---- */

/* Escribe el encabezado. Al comprimir se llama dos veces: una al inicio
   con valores provisionales y otra al final, ya con los datos reales. */
int escribirEncabezado(FILE *salida, uint32_t cantidad, uint64_t offsetIndice);

/* Escribe el arbol en preorden. */
int escribirArbol(FILE *salida, struct nodo *raiz);

/* Escribe el indice completo al final del paquete. */
int escribirIndice(FILE *salida, const Indice *indice);

/* ---- Lectura (descompresor) ---- */

/* Lee y valida el encabezado. Deja la cantidad de archivos y el offset
   del indice en los punteros. Devuelve 1 si el archivo es un paquete
   valido, 0 si no lo es o hubo un error de lectura. */
int leerEncabezado(FILE *entrada, uint32_t *cantidad, uint64_t *offsetIndice);

/* Lee el indice completo. El llamador debe liberarlo con liberarIndice(). */
int leerIndice(FILE *entrada, uint32_t cantidad, uint64_t offsetIndice,
               Indice *indice);

void liberarIndice(Indice *indice);

#endif
