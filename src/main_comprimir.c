#include <stdio.h>
#include <string.h>

#include "estrategias.h"
#include "huffman.h"

static void uso(const char *programa)
{
    fprintf(stderr, "Uso: %s <serial|fork|hilos> <dir_entrada> <dir_salida> [-v]\n",
            programa);
    fprintf(stderr, "  Comprime todos los .txt de <dir_entrada> en <dir_salida>.\n");
    fprintf(stderr, "  -v  imprime el arbol y el diccionario (solo para depurar)\n");
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        uso(argv[0]);

        return 1;
    }

    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            huffmanVerboso = 1;
        }
    }

    const char *estrategia = argv[1];
    const char *dirEntrada = argv[2];
    const char *dirSalida = argv[3];

    Estadisticas est;
    int exito;

    if (strcmp(estrategia, "serial") == 0) {
        exito = comprimirSerial(dirEntrada, dirSalida, &est);
    }
    else if (strcmp(estrategia, "fork") == 0) {
        exito = comprimirParalelo(dirEntrada, dirSalida, &est);
    }
    else if (strcmp(estrategia, "hilos") == 0) {
        exito = comprimirConcurrente(dirEntrada, dirSalida, &est);
    }
    else {
        uso(argv[0]);

        return 1;
    }

    printf("\n--- Compresion (%s) ---\n", estrategia);
    printf("Archivos procesados : %d\n", est.archivosProcesados);
    printf("Archivos correctos  : %d\n", est.archivosCorrectos);
    printf("Tiempo total        : %.6f s\n", est.segundos);
    printf("Bytes originales    : %zu\n", est.bytesOriginales);
    printf("Bytes comprimidos   : %zu\n", est.bytesComprimidos);
    printf("Razon de compresion : %.3f\n", estadisticasRazon(&est));

    if (est.mensaje[0] != '\0') {
        printf("Mensaje             : %s\n", est.mensaje);
    }

    return exito ? 0 : 1;
}
