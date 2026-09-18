#ifndef ESTRATEGIAS_H
#define ESTRATEGIAS_H

#include "estadisticas.h"

/* Las seis corridas que pide el enunciado. Todas comparten la misma firma
   para que la interfaz grafica pueda tratarlas de forma uniforme.

   dirEntrada : directorio con los .txt (comprimir) o con los .huff (descomprimir)
   dirSalida  : directorio donde se escriben los resultados
   est        : estadisticas de la corrida

   Devuelven 1 si la corrida completa tuvo exito, 0 si no. */

/* --- Version serial --- */
int comprimirSerial(const char *dirEntrada, const char *dirSalida, Estadisticas *est);
int descomprimirSerial(const char *dirEntrada, const char *dirSalida, Estadisticas *est);

/* --- Version paralela: fork() + memoria compartida --- */
int comprimirParalelo(const char *dirEntrada, const char *dirSalida, Estadisticas *est);
int descomprimirParalelo(const char *dirEntrada, const char *dirSalida, Estadisticas *est);

/* --- Version concurrente: pthread + memoria compartida --- */
int comprimirConcurrente(const char *dirEntrada, const char *dirSalida, Estadisticas *est);
int descomprimirConcurrente(const char *dirEntrada, const char *dirSalida, Estadisticas *est);

/* Cantidad de procesos o hilos que usan las versiones paralela y concurrente.
   Se ajusta con la cantidad de nucleos disponibles. */
int cantidadTrabajadores(void);

#endif
