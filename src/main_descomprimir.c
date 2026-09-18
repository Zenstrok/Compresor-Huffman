#include <stdio.h>
#include <string.h>

#include "estrategias.h"

static void uso(const char *programa)
{
    fprintf(stderr, "Uso: %s <serial|fork|hilos> <dir_entrada> <dir_salida>\n",
            programa);
    fprintf(stderr, "  Descomprime todos los .huff de <dir_entrada> en <dir_salida>.\n");
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        uso(argv[0]);

        return 1;
    }

    const char *estrategia = argv[1];
    const char *dirEntrada = argv[2];
    const char *dirSalida = argv[3];

    Estadisticas est;
    int exito;

    if (strcmp(estrategia, "serial") == 0) {
        exito = descomprimirSerial(dirEntrada, dirSalida, &est);
    }
    else if (strcmp(estrategia, "fork") == 0) {
        exito = descomprimirParalelo(dirEntrada, dirSalida, &est);
    }
    else if (strcmp(estrategia, "hilos") == 0) {
        exito = descomprimirConcurrente(dirEntrada, dirSalida, &est);
    }
    else {
        uso(argv[0]);

        return 1;
    }

    printf("\n--- Descompresion (%s) ---\n", estrategia);
    printf("Archivos procesados : %d\n", est.archivosProcesados);
    printf("Archivos correctos  : %d\n", est.archivosCorrectos);
    printf("Firmas verificadas  : %d\n", est.firmasVerificadas);
    printf("Salud               : %.1f %%\n", estadisticasSalud(&est));
    printf("Tiempo total        : %.6f s\n", est.segundos);

    if (est.mensaje[0] != '\0') {
        printf("Mensaje             : %s\n", est.mensaje);
    }

    return exito ? 0 : 1;
}
