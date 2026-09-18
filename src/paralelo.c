#include <stdio.h>
#include <unistd.h>

#include "estrategias.h"

/* ===============================================================
   PENDIENTE — Version paralela con fork() + IPC

   Esquema previsto para la COMPRESION:
     1. listarArchivos() sobre el directorio de entrada.
     2. Reservar memoria compartida con mmap(MAP_SHARED | MAP_ANONYMOUS):
        un mutex PTHREAD_PROCESS_SHARED, los acumuladores y el arreglo
        de entradas del indice.
     3. Crear cantidadTrabajadores() hijos con fork(). El hijo k procesa
        los archivos k, k+N, k+2N, ... (reparto intercalado, no por
        bloques, porque los libros tienen tamanos muy distintos).
     4. Cada hijo comprime sus archivos a bloques y necesita reservar su
        espacio dentro del paquete. Esa reserva es la REGION CRITICA:
        tomar el mutex, anotar el offset actual, avanzarlo, soltar.
     5. El padre llama wait() una vez por hijo, escribe el indice con las
        entradas que dejaron en memoria compartida y cierra el encabezado.

   Para la DESCOMPRESION es mas simple: el indice ya trae el offset de
   cada bloque, asi que cada hijo abre el paquete por su cuenta, hace
   fseek al offset que le toca y extrae su archivo. La unica region
   critica son los contadores de firmas verificadas.

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

int comprimirParalelo(const char *origen, const char *destino, Estadisticas *est)
{
    (void)origen;
    (void)destino;

    estadisticasIniciar(est);
    snprintf(est->mensaje, sizeof(est->mensaje),
             "Version paralela pendiente de implementar (src/paralelo.c)");

    return 0;
}

int descomprimirParalelo(const char *origen, const char *destino, Estadisticas *est)
{
    (void)origen;
    (void)destino;

    estadisticasIniciar(est);
    snprintf(est->mensaje, sizeof(est->mensaje),
             "Version paralela pendiente de implementar (src/paralelo.c)");

    return 0;
}
