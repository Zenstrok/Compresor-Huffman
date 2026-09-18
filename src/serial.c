#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "estrategias.h"
#include "compresor.h"
#include "descompresor.h"
#include "directorio.h"

static size_t tamanoDe(const char *ruta)
{
    struct stat info;

    if (stat(ruta, &info) != 0) {
        return 0;
    }

    return (size_t)info.st_size;
}

int comprimirSerial(const char *dirEntrada, const char *dirSalida, Estadisticas *est)
{
    estadisticasIniciar(est);

    if (!asegurarDirectorio(dirSalida)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo crear el directorio de salida: %s", dirSalida);

        return 0;
    }

    ListaArchivos lista;

    if (!listarArchivos(dirEntrada, ".txt", &lista)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo leer el directorio: %s", dirEntrada);

        return 0;
    }

    double inicio = relojSegundos();

    for (int i = 0; i < lista.cantidad; i++) {
        char destino[4096];

        if (!rutaDestino(destino, sizeof(destino), dirSalida, lista.rutas[i], ".huff")) {
            continue;
        }

        est->archivosProcesados++;

        if (comprimirArchivo(lista.rutas[i], destino)) {
            est->archivosCorrectos++;
            est->bytesOriginales += tamanoDe(lista.rutas[i]);
            est->bytesComprimidos += tamanoDe(destino);
        }
    }

    est->segundos = relojSegundos() - inicio;
    est->ok = est->archivosProcesados == est->archivosCorrectos;

    if (!est->ok) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "%d de %d archivos fallaron al comprimir",
                 est->archivosProcesados - est->archivosCorrectos,
                 est->archivosProcesados);
    }

    liberarListaArchivos(&lista);

    return est->ok;
}

int descomprimirSerial(const char *dirEntrada, const char *dirSalida, Estadisticas *est)
{
    estadisticasIniciar(est);

    if (!asegurarDirectorio(dirSalida)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo crear el directorio de salida: %s", dirSalida);

        return 0;
    }

    ListaArchivos lista;

    if (!listarArchivos(dirEntrada, ".huff", &lista)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo leer el directorio: %s", dirEntrada);

        return 0;
    }

    double inicio = relojSegundos();

    for (int i = 0; i < lista.cantidad; i++) {
        char destino[4096];

        if (!rutaDestino(destino, sizeof(destino), dirSalida, lista.rutas[i], ".txt")) {
            continue;
        }

        est->archivosProcesados++;

        int coincide = 0;

        if (descomprimirArchivo(lista.rutas[i], destino, &coincide)) {
            est->archivosCorrectos++;
            est->bytesComprimidos += tamanoDe(lista.rutas[i]);
            est->bytesOriginales += tamanoDe(destino);

            if (coincide) {
                est->firmasVerificadas++;
            }
        }
    }

    est->segundos = relojSegundos() - inicio;
    est->ok = 0 < est->archivosProcesados &&
              est->archivosProcesados == est->archivosCorrectos;

    if (!est->ok) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "Descompresor pendiente de implementar (src/descompresor.c)");
    }

    liberarListaArchivos(&lista);

    return est->ok;
}
