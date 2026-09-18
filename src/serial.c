#include "estrategias.h"
#include "compresor.h"
#include "descompresor.h"

int comprimirSerial(const char *origen, const char *destino, Estadisticas *est)
{
    return comprimirDirectorio(origen, destino, est);
}

int descomprimirSerial(const char *origen, const char *destino, Estadisticas *est)
{
    return descomprimirPaquete(origen, destino, est);
}
