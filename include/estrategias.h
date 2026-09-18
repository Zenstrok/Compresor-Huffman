#ifndef ESTRATEGIAS_H
#define ESTRATEGIAS_H

#include "estadisticas.h"

/* Las seis corridas que pide el enunciado. Todas comparten la misma firma
   para que la interfaz grafica pueda tratarlas de forma uniforme.

   Al comprimir:
     origen  = directorio con los archivos .txt
     destino = ruta del paquete .huff que se va a crear

   Al descomprimir:
     origen  = ruta del paquete .huff
     destino = directorio donde se expanden los archivos

   Devuelven 1 si la corrida completa tuvo exito, 0 si no. */

/* --- Version serial --- */
int comprimirSerial(const char *origen, const char *destino, Estadisticas *est);
int descomprimirSerial(const char *origen, const char *destino, Estadisticas *est);

/* --- Version paralela: fork() + IPC --- */
int comprimirParalelo(const char *origen, const char *destino, Estadisticas *est);
int descomprimirParalelo(const char *origen, const char *destino, Estadisticas *est);

/* --- Version concurrente: pthread + memoria compartida --- */
int comprimirConcurrente(const char *origen, const char *destino, Estadisticas *est);
int descomprimirConcurrente(const char *origen, const char *destino, Estadisticas *est);

/* Cantidad de procesos o hilos que usan las versiones paralela y concurrente. */
int cantidadTrabajadores(void);

#endif
