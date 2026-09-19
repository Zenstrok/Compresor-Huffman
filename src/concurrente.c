#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estrategias.h"
#include "compresor.h"
#include "descompresor.h"
#include "directorio.h"
#include "formato.h"

/* ===============================================================
   Version concurrente: pthread + memoria compartida

   Mismo reparto intercalado que la version con fork(): el hilo k
   procesa los archivos k, k+N, k+2N, ...

   La diferencia de fondo con fork() es que los hilos comparten el
   espacio de memoria del proceso por naturaleza. Por eso aqui los
   acumuladores son variables normales en vez de mmap(), y el mutex
   se declara sin atributos: el valor por omision
   (PTHREAD_PROCESS_PRIVATE) ya alcanza.
   =============================================================== */

/* Acumuladores compartidos por todos los hilos. */
typedef struct {
    pthread_mutex_t mutex;
    int archivosCorrectos;
    int firmasVerificadas;
    size_t bytesOriginales;
    size_t bytesComprimidos;
} Compartido;

typedef struct {
    int k;
    int trabajadores;
    const ListaArchivos *lista;
    FILE *salida;
    Compartido *compartido;
    EntradaIndice *indiceCompartido;
} ArgumentoCompresion;

typedef struct {
    int k;
    int trabajadores;
    const Indice *indice;
    const char *rutaPaquete;
    const char *dirSalida;
    Compartido *compartido;
} ArgumentoDescompresion;

static void iniciarCompartido(Compartido *compartido)
{
    pthread_mutex_init(&compartido->mutex, NULL);

    compartido->archivosCorrectos = 0;
    compartido->firmasVerificadas = 0;
    compartido->bytesOriginales = 0;
    compartido->bytesComprimidos = 0;
}

/* ---------------------------------------------------------------
   Compresion
   --------------------------------------------------------------- */

static void *trabajarCompresion(void *puntero)
{
    ArgumentoCompresion *arg = puntero;
    Compartido *compartido = arg->compartido;

    int correctos = 0;
    size_t originales = 0;
    size_t comprimidos = 0;

    for (int i = arg->k; i < arg->lista->cantidad; i += arg->trabajadores) {
        char *bloque = NULL;
        size_t tamanoBloque = 0;
        EntradaIndice entrada;

        /* Comprimir a memoria, FUERA del candado: es la parte cara y no
           toca ningun recurso compartido. */
        if (!comprimirArchivoABuffer(arg->lista->rutas[i], &bloque,
                                     &tamanoBloque, &entrada)) {
            continue;
        }

        /* ---------- REGION CRITICA ----------
           Reservar el espacio al final del paquete y escribir el bloque.
           A diferencia de la version con fork(), aqui si se puede usar un
           FILE * compartido: los hilos comparten el buffer de stdio, no
           tienen una copia cada uno. */
        pthread_mutex_lock(&compartido->mutex);

        long offset = -1;
        int escrito = 0;

        if (fseek(arg->salida, 0, SEEK_END) == 0) {
            offset = ftell(arg->salida);

            if (0 <= offset) {
                escrito = fwrite(bloque, 1, tamanoBloque, arg->salida) == tamanoBloque;
            }
        }

        pthread_mutex_unlock(&compartido->mutex);
        /* ------------------------------------ */

        free(bloque);

        if (!escrito) {
            continue;
        }

        /* Cada hilo escribe solo en las posiciones que le tocan del arreglo,
           asi que aqui no hace falta candado. */
        entrada.offsetBloque = (uint64_t)offset;
        arg->indiceCompartido[i] = entrada;

        correctos++;
        originales += (size_t)entrada.tamanoOriginal;
        comprimidos += tamanoBloque;
    }

    /* Un solo bloqueo al final para volcar los subtotales. */
    pthread_mutex_lock(&compartido->mutex);

    compartido->archivosCorrectos += correctos;
    compartido->bytesOriginales += originales;
    compartido->bytesComprimidos += comprimidos;

    pthread_mutex_unlock(&compartido->mutex);

    return NULL;
}

int comprimirConcurrente(const char *origen, const char *destino, Estadisticas *est)
{
    estadisticasIniciar(est);

    ListaArchivos lista;

    if (!listarArchivos(origen, ".txt", &lista)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo leer el directorio: %s", origen);

        return 0;
    }

    if (lista.cantidad == 0) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se encontraron archivos .txt en: %s", origen);
        liberarListaArchivos(&lista);

        return 0;
    }

    int trabajadores = cantidadTrabajadores();

    if (lista.cantidad < trabajadores) {
        trabajadores = lista.cantidad;
    }

    Compartido compartido;
    EntradaIndice *indiceCompartido =
        calloc((size_t)lista.cantidad, sizeof(EntradaIndice));
    pthread_t *hilos = calloc((size_t)trabajadores, sizeof(pthread_t));
    ArgumentoCompresion *argumentos =
        calloc((size_t)trabajadores, sizeof(ArgumentoCompresion));

    if (indiceCompartido == NULL || hilos == NULL || argumentos == NULL) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo reservar memoria");
        free(indiceCompartido);
        free(hilos);
        free(argumentos);
        liberarListaArchivos(&lista);

        return 0;
    }

    iniciarCompartido(&compartido);

    FILE *salida = fopen(destino, "w+b");

    if (salida == NULL) {
        perror("Error: no se pudo crear el paquete comprimido");
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo crear el paquete: %s", destino);
        free(indiceCompartido);
        free(hilos);
        free(argumentos);
        liberarListaArchivos(&lista);

        return 0;
    }

    double inicio = relojSegundos();

    /* Encabezado provisional: reserva los primeros bytes para que los
       hilos escriban despues de el. Se reescribe al final. */
    if (!escribirEncabezado(salida, 0, 0)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo escribir el encabezado del paquete");
        fclose(salida);
        free(indiceCompartido);
        free(hilos);
        free(argumentos);
        liberarListaArchivos(&lista);

        return 0;
    }

    int creados = 0;

    for (int k = 0; k < trabajadores; k++) {
        argumentos[k].k = k;
        argumentos[k].trabajadores = trabajadores;
        argumentos[k].lista = &lista;
        argumentos[k].salida = salida;
        argumentos[k].compartido = &compartido;
        argumentos[k].indiceCompartido = indiceCompartido;

        if (pthread_create(&hilos[k], NULL, trabajarCompresion, &argumentos[k]) != 0) {
            perror("Error: pthread_create fallo");

            break;
        }

        creados++;
    }

    /* Esperar a TODOS los hilos antes de leer los acumuladores */
    for (int k = 0; k < creados; k++) {
        pthread_join(hilos[k], NULL);
    }

    est->archivosProcesados = lista.cantidad;
    est->archivosCorrectos = compartido.archivosCorrectos;
    est->bytesOriginales = compartido.bytesOriginales;

    /* Armar el indice en el orden original de los archivos */
    Indice indice;

    indice.entradas = calloc((size_t)lista.cantidad, sizeof(EntradaIndice));
    indice.cantidad = 0;

    if (indice.entradas != NULL) {
        for (int i = 0; i < lista.cantidad; i++) {
            if (indiceCompartido[i].nombre[0] != '\0') {
                indice.entradas[indice.cantidad] = indiceCompartido[i];
                indice.cantidad++;
            }
        }
    }

    /* Escribir el indice y cerrar el encabezado */
    if (indice.entradas == NULL || fseek(salida, 0, SEEK_END) != 0) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo cerrar el paquete");
    }
    else {
        long offsetIndice = ftell(salida);

        if (0 <= offsetIndice && escribirIndice(salida, &indice) &&
            fseek(salida, 0, SEEK_SET) == 0 &&
            escribirEncabezado(salida, indice.cantidad, (uint64_t)offsetIndice)) {

            if (fseek(salida, 0, SEEK_END) == 0) {
                long total = ftell(salida);

                est->bytesComprimidos = 0 <= total ? (size_t)total
                                                   : (size_t)offsetIndice;
            }

            est->segundos = relojSegundos() - inicio;
            est->ok = 0 < est->archivosCorrectos &&
                      est->archivosProcesados == est->archivosCorrectos;
        }
        else {
            snprintf(est->mensaje, sizeof(est->mensaje),
                     "No se pudo escribir el indice del paquete");
        }
    }

    if (!est->ok && est->mensaje[0] == '\0') {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "%d de %d archivos no se pudieron comprimir",
                 est->archivosProcesados - est->archivosCorrectos,
                 est->archivosProcesados);
    }

    fclose(salida);
    pthread_mutex_destroy(&compartido.mutex);
    free(indice.entradas);
    free(indiceCompartido);
    free(hilos);
    free(argumentos);
    liberarListaArchivos(&lista);

    return est->ok;
}

/* ---------------------------------------------------------------
   Descompresion
   --------------------------------------------------------------- */

static void *trabajarDescompresion(void *puntero)
{
    ArgumentoDescompresion *arg = puntero;
    Compartido *compartido = arg->compartido;

    /* Cada hilo abre el paquete por su cuenta. Aqui NO se puede compartir
       un FILE *: cada hilo hace fseek a un offset distinto y se pisarian
       la posicion de lectura entre ellos. */
    FILE *entrada = fopen(arg->rutaPaquete, "rb");

    if (entrada == NULL) {
        return NULL;
    }

    int correctos = 0;
    int firmas = 0;
    size_t originales = 0;
    size_t comprimidos = 0;

    for (uint32_t i = (uint32_t)arg->k; i < arg->indice->cantidad;
         i += (uint32_t)arg->trabajadores) {

        const EntradaIndice *meta = &arg->indice->entradas[i];
        char ruta[4096];

        if ((int)sizeof(ruta) <=
            snprintf(ruta, sizeof(ruta), "%s/%s", arg->dirSalida, meta->nombre)) {
            continue;
        }

        int coincide = 0;

        if (extraerArchivo(entrada, meta, ruta, &coincide)) {
            correctos++;
            originales += (size_t)meta->tamanoOriginal;
            comprimidos += (size_t)meta->tamanoBloque;

            if (coincide) {
                firmas++;
            }
        }
    }

    fclose(entrada);

    /* ---------- REGION CRITICA ---------- */
    pthread_mutex_lock(&compartido->mutex);

    compartido->archivosCorrectos += correctos;
    compartido->firmasVerificadas += firmas;
    compartido->bytesOriginales += originales;
    compartido->bytesComprimidos += comprimidos;

    pthread_mutex_unlock(&compartido->mutex);
    /* ------------------------------------ */

    return NULL;
}

int descomprimirConcurrente(const char *origen, const char *destino, Estadisticas *est)
{
    estadisticasIniciar(est);

    if (!asegurarDirectorio(destino)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo crear el directorio de salida: %s", destino);

        return 0;
    }

    FILE *entrada = fopen(origen, "rb");

    if (entrada == NULL) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo abrir el paquete: %s", origen);

        return 0;
    }

    double inicio = relojSegundos();

    uint32_t cantidad = 0;
    uint64_t offsetIndice = 0;
    Indice indice;

    if (!leerEncabezado(entrada, &cantidad, &offsetIndice)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "El archivo no es un paquete HUFFPKG1 valido: %s", origen);
        fclose(entrada);

        return 0;
    }

    if (!leerIndice(entrada, cantidad, offsetIndice, &indice)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo leer el indice del paquete");
        fclose(entrada);

        return 0;
    }

    fclose(entrada);

    int trabajadores = cantidadTrabajadores();

    if ((uint32_t)trabajadores > indice.cantidad) {
        trabajadores = (int)indice.cantidad;
    }

    Compartido compartido;
    pthread_t *hilos = calloc((size_t)trabajadores, sizeof(pthread_t));
    ArgumentoDescompresion *argumentos =
        calloc((size_t)trabajadores, sizeof(ArgumentoDescompresion));

    if (hilos == NULL || argumentos == NULL) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo reservar memoria");
        free(hilos);
        free(argumentos);
        liberarIndice(&indice);

        return 0;
    }

    iniciarCompartido(&compartido);

    int creados = 0;

    for (int k = 0; k < trabajadores; k++) {
        argumentos[k].k = k;
        argumentos[k].trabajadores = trabajadores;
        argumentos[k].indice = &indice;
        argumentos[k].rutaPaquete = origen;
        argumentos[k].dirSalida = destino;
        argumentos[k].compartido = &compartido;

        if (pthread_create(&hilos[k], NULL, trabajarDescompresion,
                           &argumentos[k]) != 0) {
            perror("Error: pthread_create fallo");

            break;
        }

        creados++;
    }

    for (int k = 0; k < creados; k++) {
        pthread_join(hilos[k], NULL);
    }

    est->segundos = relojSegundos() - inicio;
    est->archivosProcesados = (int)indice.cantidad;
    est->archivosCorrectos = compartido.archivosCorrectos;
    est->firmasVerificadas = compartido.firmasVerificadas;
    est->bytesOriginales = compartido.bytesOriginales;
    est->bytesComprimidos = compartido.bytesComprimidos;
    est->ok = 0 < est->archivosProcesados &&
              est->archivosProcesados == est->archivosCorrectos;

    if (!est->ok) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "%d de %d archivos no se pudieron extraer",
                 est->archivosProcesados - est->archivosCorrectos,
                 est->archivosProcesados);
    }
    else if (est->firmasVerificadas != est->archivosProcesados) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "%d de %d firmas MD5 no coinciden",
                 est->archivosProcesados - est->firmasVerificadas,
                 est->archivosProcesados);
    }

    pthread_mutex_destroy(&compartido.mutex);
    free(hilos);
    free(argumentos);
    liberarIndice(&indice);

    return est->ok;
}