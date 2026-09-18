#include <stdio.h>
#include <stdlib.h>

#include "archivo.h"

int cargarArchivo(const char *ruta, unsigned char **datos, size_t *tamano)
{
    FILE *archivo = fopen(ruta, "rb");

    if (archivo == NULL) {
        perror("Error de cargar: No se pudo abrir el archivo");

        return 0;
    }

    if (fseek(archivo, 0, SEEK_END) != 0) {
        fclose(archivo);

        return 0;
    }

    long tamanoArchivo = ftell(archivo);

    if (tamanoArchivo < 0) {
        fclose(archivo);

        return 0;
    }

    rewind(archivo);

    *tamano = (size_t)tamanoArchivo;

    /* Archivo vacio: no hay nada que leer */
    if (*tamano == 0) {
        *datos = NULL;
        fclose(archivo);

        return 1;
    }

    *datos = malloc(*tamano);

    if (*datos == NULL) {
        fclose(archivo);

        return 0;
    }

    size_t leidos = fread(*datos, 1, *tamano, archivo);

    fclose(archivo);

    if (leidos != *tamano) {
        free(*datos);
        *datos = NULL;

        return 0;
    }

    return 1;
}
