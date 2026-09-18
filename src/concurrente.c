#include <stdio.h>

#include "estrategias.h"

/* ===============================================================
   PENDIENTE — Version concurrente con pthread + memoria compartida

   Esquema previsto:
     1. listarArchivos() sobre el directorio de entrada.
     2. Acumuladores como variables compartidas del proceso, protegidas
        por un pthread_mutex_t normal: aqui NO hace falta
        PTHREAD_PROCESS_SHARED, porque todos los hilos viven dentro
        del mismo proceso.
     3. Crear cantidadTrabajadores() hilos con pthread_create(). El hilo k
        procesa los archivos k, k+N, k+2N, ... igual que la version con fork.
     4. Cada hilo acumula en variables locales de su funcion y toma el
        mutex una sola vez al final.
     5. pthread_join() por cada hilo antes de leer los acumuladores.

   OJO: ningun modulo puede usar variables globales mutables o los hilos
   se pisan entre si. Por eso huffman() recibe el diccionario por
   parametro en vez de tenerlo como global.
   =============================================================== */

int comprimirConcurrente(const char *dirEntrada, const char *dirSalida, Estadisticas *est)
{
    (void)dirEntrada;
    (void)dirSalida;

    estadisticasIniciar(est);
    snprintf(est->mensaje, sizeof(est->mensaje),
             "Version concurrente pendiente de implementar (src/concurrente.c)");

    return 0;
}

int descomprimirConcurrente(const char *dirEntrada, const char *dirSalida, Estadisticas *est)
{
    (void)dirEntrada;
    (void)dirSalida;

    estadisticasIniciar(est);
    snprintf(est->mensaje, sizeof(est->mensaje),
             "Version concurrente pendiente de implementar (src/concurrente.c)");

    return 0;
}
