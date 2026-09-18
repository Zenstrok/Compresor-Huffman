#include <stdio.h>

#include "descompresor.h"

/* ===============================================================
   PENDIENTE — Descompresor (parte de Mario)

   Pasos que debe seguir descomprimirArchivo():

     1. Abrir el .huff y validar los primeros 6 bytes contra MAGIC_HUFF.
     2. Leer los 16 bytes del MD5 guardado.
     3. Leer el uint64_t con el tamano original (condicion de parada).
     4. Leer el byte tieneArbol. Si es 0, el original estaba vacio:
        crear el archivo de salida vacio y terminar.
     5. Reconstruir el arbol leyendo en preorden:
          0x00 -> nodo interno, llamar recursivo izquierda y luego derecha
          0x01 -> leer 1 byte mas y crear la hoja
     6. Caso especial: si la raiz es una hoja (archivo de un solo caracter
        distinto), escribir ese caracter tamanoOriginal veces y terminar.
        Si no se trata aparte, el decodificador nunca avanza.
     7. Decodificar el flujo de bits, de bit 7 a bit 0 dentro de cada byte.
        Partir de la raiz; 0 va a la izquierda y 1 a la derecha. Al llegar
        a una hoja, escribir su caracter y volver a la raiz.
        PARAR al escribir tamanoOriginal bytes, no al agotar el archivo:
        el ultimo byte trae relleno de ceros.
     8. Cerrar la salida, calcular su MD5 con calcularMD5() y compararlo
        contra el del paso 2. Dejar el resultado en *firmaCoincide.

   El formato completo esta documentado en include/formato.h.
   Las funciones de lectura del formato (leerMetadatos, leerArbol) van
   en src/formato.c, al lado de las de escritura.
   =============================================================== */

int descomprimirArchivo(const char *rutaComprimida, const char *rutaSalida,
                        int *firmaCoincide)
{
    (void)rutaComprimida;
    (void)rutaSalida;

    if (firmaCoincide != NULL) {
        *firmaCoincide = 0;
    }

    fprintf(stderr, "descomprimirArchivo(): pendiente de implementar.\n");

    return 0;
}
