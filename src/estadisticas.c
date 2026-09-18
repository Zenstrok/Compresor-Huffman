#include <string.h>
#include <time.h>

#include "estadisticas.h"

void estadisticasIniciar(Estadisticas *est)
{
    memset(est, 0, sizeof(Estadisticas));
}

double estadisticasSalud(const Estadisticas *est)
{
    if (est->archivosProcesados == 0) {
        return 0.0;
    }

    return 100.0 * (double)est->firmasVerificadas / (double)est->archivosProcesados;
}

double estadisticasRazon(const Estadisticas *est)
{
    if (est->bytesOriginales == 0) {
        return 0.0;
    }

    return (double)est->bytesComprimidos / (double)est->bytesOriginales;
}

double estadisticasAceleracion(double segundosSerial, double segundosActual)
{
    if (segundosSerial <= 0.0) {
        return 0.0;
    }

    return 100.0 * (segundosSerial - segundosActual) / segundosSerial;
}

double relojSegundos(void)
{
    struct timespec ahora;

    clock_gettime(CLOCK_MONOTONIC, &ahora);

    return (double)ahora.tv_sec + (double)ahora.tv_nsec / 1000000000.0;
}
