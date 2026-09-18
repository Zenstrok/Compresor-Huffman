#include <stdio.h>
#include <unistd.h>

#include "estrategias.h"

/* ===============================================================
   PENDIENTE — Version paralela con fork() + IPC

   Esquema previsto:
     1. listarArchivos() sobre el directorio de entrada.
     2. Reservar memoria compartida con mmap(MAP_SHARED | MAP_ANONYMOUS)
        con un mutex PTHREAD_PROCESS_SHARED y los acumuladores
        (archivos correctos, firmas verificadas, bytes).
     3. Crear cantidadTrabajadores() hijos con fork(). El hijo k procesa
        los archivos k, k+N, k+2N, ... (reparto intercalado, no por
        bloques, porque los libros tienen tamanos muy distintos).
     4. Cada hijo acumula en variables LOCALES y al terminar toma el
        mutex una sola vez para sumar al total compartido.
     5. El padre llama wait() una vez por hijo y recien despues lee
        los acumuladores.

   El cronometro va antes del primer fork() y despues del ultimo wait():
   el tiempo de la corrida es el del hijo mas lento.
   =============================================================== */

int cantidadTrabajadores(void)
{
    long nucleos = sysconf(_SC_NPROCESSORS_ONLN);

    if (nucleos < 1) {
        return 1;
    }

    return (int)nucleos;
}

int comprimirParalelo(const char *dirEntrada, const char *dirSalida, Estadisticas *est)
{
    (void)dirEntrada;
    (void)dirSalida;

    estadisticasIniciar(est);
    snprintf(est->mensaje, sizeof(est->mensaje),
             "Version paralela pendiente de implementar (src/paralelo.c)");

    return 0;
}

int descomprimirParalelo(const char *dirEntrada, const char *dirSalida, Estadisticas *est)
{
    (void)dirEntrada;
    (void)dirSalida;

    estadisticasIniciar(est);
    snprintf(est->mensaje, sizeof(est->mensaje),
             "Version paralela pendiente de implementar (src/paralelo.c)");

    return 0;
}
