#ifndef ESTADISTICAS_H
#define ESTADISTICAS_H

#include <stddef.h>

/* Resultado de una corrida completa sobre un directorio.
   Es la estructura que la interfaz grafica muestra en su tabla. */
typedef struct {
    int archivosProcesados;      /* cuantos archivos se intentaron          */
    int archivosCorrectos;       /* cuantos terminaron sin error            */
    int firmasVerificadas;       /* cuantos MD5 coincidieron (descompresion)*/
    double segundos;             /* tiempo total de la corrida              */
    size_t bytesOriginales;      /* suma de los tamanos originales          */
    size_t bytesComprimidos;     /* suma de los tamanos .huff               */
    int ok;                      /* 1 si la corrida completa tuvo exito     */
    char mensaje[256];           /* detalle del error, si lo hubo           */
} Estadisticas;

/* Deja la estructura en ceros y el mensaje vacio. */
void estadisticasIniciar(Estadisticas *est);

/* Porcentaje de salud: firmas verificadas / archivos procesados * 100.
   Devuelve 0 si no se proceso ningun archivo. */
double estadisticasSalud(const Estadisticas *est);

/* Razon de compresion: bytes comprimidos / bytes originales.
   Devuelve 0 si no hay bytes originales. */
double estadisticasRazon(const Estadisticas *est);

/* Aceleracion porcentual respecto a la corrida serial:
   (serial - actual) / serial * 100. Positiva significa mas rapido. */
double estadisticasAceleracion(double segundosSerial, double segundosActual);

/* Reloj monotono en segundos, para medir intervalos. */
double relojSegundos(void);

#endif
