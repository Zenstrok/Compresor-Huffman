#include <stdio.h>

#include "descompresor.h"

/* ===============================================================
   PENDIENTE — Descompresor

   El formato del paquete esta documentado en include/formato.h.
   Las funciones leerEncabezado() y leerIndice() ya estan implementadas
   en src/formato.c: con ellas se abre el paquete y se obtiene la lista
   de archivos que contiene, con su nombre, tamano original, MD5, offset
   y tamano de bloque.

   --- leerArbol(entrada) ---
     Lee un byte:
       0x01 -> leer 1 byte mas y devolver una hoja con ese caracter
       0x00 -> crear un nodo interno, llamar recursivo para el hijo
               izquierdo y despues para el derecho
     Cualquier otro valor es un paquete corrupto: devolver NULL.

   --- extraerArchivo(entrada, meta, rutaSalida, firmaCoincide) ---
     1. fseek(entrada, meta->offsetBloque, SEEK_SET).
     2. Leer el byte tieneArbol. Si es 0, el original estaba vacio:
        crear el archivo de salida vacio y terminar.
     3. raiz = leerArbol(entrada).
     4. CASO ESPECIAL: si la raiz es una hoja (archivo con un solo
        caracter distinto), escribir ese caracter meta->tamanoOriginal
        veces y terminar. Si no se trata aparte, el decodificador se
        queda en la raiz y nunca avanza.
     5. Decodificar: leer byte por byte, y dentro de cada byte del bit 7
        al bit 0. Partir de la raiz; 0 va a la izquierda y 1 a la derecha.
        Al llegar a una hoja, escribir su caracter y volver a la raiz.
        PARAR al escribir meta->tamanoOriginal bytes, no al agotar el
        bloque: el ultimo byte trae relleno de ceros.
     6. Cerrar la salida, calcular su MD5 con calcularMD5() y compararlo
        con meta->firma usando memcmp. Dejar el resultado en
        *firmaCoincide.
     7. liberarArbol(raiz).

   --- descomprimirPaquete(rutaPaquete, dirSalida, est) ---
     1. asegurarDirectorio(dirSalida).
     2. Abrir el paquete, leerEncabezado() y leerIndice().
     3. Arrancar el cronometro con relojSegundos().
     4. Por cada entrada del indice: armar la ruta de salida como
        dirSalida + "/" + meta->nombre y llamar a extraerArchivo().
        Contar archivosProcesados, archivosCorrectos y firmasVerificadas,
        y acumular bytesOriginales y bytesComprimidos.
     5. Parar el cronometro, llenar est->segundos y est->ok.
     6. liberarIndice() y cerrar el paquete.
   =============================================================== */

struct nodo *leerArbol(FILE *entrada)
{
    (void)entrada;

    return NULL;
}

int extraerArchivo(FILE *entrada, const EntradaIndice *meta,
                   const char *rutaSalida, int *firmaCoincide)
{
    (void)entrada;
    (void)meta;
    (void)rutaSalida;

    if (firmaCoincide != NULL) {
        *firmaCoincide = 0;
    }

    return 0;
}

int descomprimirPaquete(const char *rutaPaquete, const char *dirSalida,
                        Estadisticas *est)
{
    (void)rutaPaquete;
    (void)dirSalida;

    estadisticasIniciar(est);
    snprintf(est->mensaje, sizeof(est->mensaje),
             "Descompresor pendiente de implementar (src/descompresor.c)");

    return 0;
}
