#include <stdio.h>

#include "estrategias.h"

/* ===============================================================
   PENDIENTE — Version concurrente con pthread + memoria compartida

   Mismo reparto que la version paralela, pero con hilos:
     1. listarArchivos() sobre el directorio de entrada.
     2. Los acumuladores y el arreglo de entradas del indice son
        variables compartidas del proceso, protegidas por un
        pthread_mutex_t normal: aqui NO hace falta
        PTHREAD_PROCESS_SHARED, porque todos los hilos viven dentro
        del mismo proceso.
     3. Crear cantidadTrabajadores() hilos con pthread_create(). El hilo k
        procesa los archivos k, k+N, k+2N, ...
     4. La region critica es la misma: reservar el espacio de cada bloque
        dentro del paquete, y actualizar los contadores.
     5. pthread_join() por cada hilo antes de escribir el indice.

   OJO: ningun modulo puede usar variables globales mutables o los hilos
   se pisan entre si. Por eso huffman() recibe el diccionario por
   parametro en vez de tenerlo como global.
   =============================================================== */

int comprimirConcurrente(const char *origen, const char *destino, Estadisticas *est)
{
    (void)origen;
    (void)destino;

    estadisticasIniciar(est);
    snprintf(est->mensaje, sizeof(est->mensaje),
             "Version concurrente pendiente de implementar (src/concurrente.c)");

    return 0;
}

int descomprimirConcurrente(const char *origen, const char *destino, Estadisticas *est)
{
    (void)origen;
    (void)destino;

    estadisticasIniciar(est);
    snprintf(est->mensaje, sizeof(est->mensaje),
             "Version concurrente pendiente de implementar (src/concurrente.c)");

    return 0;
}
