#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "estrategias.h"
#include "compresor.h"
#include "descompresor.h"
#include "directorio.h"
#include "formato.h"

/* ===============================================================
   Version paralela: fork() + memoria compartida como estrategia de IPC

   Reparto intercalado: el hijo k procesa los archivos k, k+N, k+2N, ...
   No se reparte por bloques contiguos porque los libros tienen tamanos
   muy distintos y el tiempo total lo marca el hijo mas lento.
   =============================================================== */

/* Acumuladores compartidos entre el padre y todos los hijos. */
typedef struct {
    pthread_mutex_t mutex;
    int archivosCorrectos;
    int firmasVerificadas;
    size_t bytesOriginales;
    size_t bytesComprimidos;
} Compartido;

int cantidadTrabajadores(void)
{
    long nucleos = sysconf(_SC_NPROCESSORS_ONLN);

    if (nucleos < 1) {
        return 1;
    }

    return (int)nucleos;
}

/* ---------------------------------------------------------------
   Memoria compartida
   --------------------------------------------------------------- */

static void *reservarCompartida(size_t tamano)
{
    void *region = mmap(NULL, tamano, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    return region == MAP_FAILED ? NULL : region;
}

/* Inicializa el mutex para que funcione ENTRE PROCESOS y no solo entre
   hilos. Sin PTHREAD_PROCESS_SHARED el candado no protege nada despues
   del fork(). */
static int iniciarCompartido(Compartido *compartido)
{
    pthread_mutexattr_t atributos;

    if (pthread_mutexattr_init(&atributos) != 0) {
        return 0;
    }

    if (pthread_mutexattr_setpshared(&atributos, PTHREAD_PROCESS_SHARED) != 0) {
        pthread_mutexattr_destroy(&atributos);

        return 0;
    }

    int resultado = pthread_mutex_init(&compartido->mutex, &atributos) == 0;

    pthread_mutexattr_destroy(&atributos);

    compartido->archivosCorrectos = 0;
    compartido->firmasVerificadas = 0;
    compartido->bytesOriginales = 0;
    compartido->bytesComprimidos = 0;

    return resultado;
}

/* ---------------------------------------------------------------
   Compresion
   --------------------------------------------------------------- */

/* Trabajo de un hijo: comprime los archivos que le tocan, y por cada uno
   toma el candado para reservar su espacio al final del paquete. */
static void trabajarCompresion(int k, int trabajadores, const ListaArchivos *lista,
                               int descriptor, Compartido *compartido,
                               EntradaIndice *indiceCompartido)
{
    int correctos = 0;
    size_t originales = 0;
    size_t comprimidos = 0;

    for (int i = k; i < lista->cantidad; i += trabajadores) {
        char *bloque = NULL;
        size_t tamanoBloque = 0;
        EntradaIndice entrada;

        if (!comprimirArchivoABuffer(lista->rutas[i], &bloque, &tamanoBloque,
                                     &entrada)) {
            continue;
        }

        /* ---------- REGION CRITICA ----------
           Reservar el espacio al final del paquete y escribir el bloque.
           Debe ser atomico: si dos hijos consultaran el final al mismo
           tiempo, los dos escribirian sobre la misma posicion. */
        pthread_mutex_lock(&compartido->mutex);

        off_t offset = lseek(descriptor, 0, SEEK_END);
        int escrito = 0;

        if (0 <= offset) {
            ssize_t bytes = write(descriptor, bloque, tamanoBloque);

            escrito = bytes == (ssize_t)tamanoBloque;
        }

        pthread_mutex_unlock(&compartido->mutex);
        /* ------------------------------------ */

        free(bloque);

        if (!escrito) {
            continue;
        }

        /* Cada hijo escribe solo en las posiciones que le tocan del arreglo
           compartido, asi que aqui no hace falta candado. */
        entrada.offsetBloque = (uint64_t)offset;
        indiceCompartido[i] = entrada;

        correctos++;
        originales += (size_t)entrada.tamanoOriginal;
        comprimidos += tamanoBloque;
    }

    /* Un solo bloqueo al final para volcar los subtotales, en vez de uno
       por archivo. */
    pthread_mutex_lock(&compartido->mutex);

    compartido->archivosCorrectos += correctos;
    compartido->bytesOriginales += originales;
    compartido->bytesComprimidos += comprimidos;

    pthread_mutex_unlock(&compartido->mutex);
}

int comprimirParalelo(const char *origen, const char *destino, Estadisticas *est)
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

    Compartido *compartido = reservarCompartida(sizeof(Compartido));
    EntradaIndice *indiceCompartido =
        reservarCompartida(sizeof(EntradaIndice) * (size_t)lista.cantidad);

    if (compartido == NULL || indiceCompartido == NULL ||
        !iniciarCompartido(compartido)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo reservar la memoria compartida");
        liberarListaArchivos(&lista);

        return 0;
    }

    memset(indiceCompartido, 0, sizeof(EntradaIndice) * (size_t)lista.cantidad);

    /* Se usa un descriptor crudo y no un FILE *: despues del fork() cada
       hijo tendria su propia copia del buffer de stdio y los bloques
       saldrian entremezclados. Con write() sobre el descriptor heredado
       todos comparten la misma posicion en el archivo. */
    int descriptor = open(destino, O_CREAT | O_TRUNC | O_RDWR, 0644);

    if (descriptor < 0) {
        perror("Error: no se pudo crear el paquete comprimido");
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo crear el paquete: %s", destino);
        liberarListaArchivos(&lista);

        return 0;
    }

    double inicio = relojSegundos();

    /* Encabezado provisional: reserva los 20 bytes iniciales para que los
       hijos escriban despues de el. Se reescribe al final. */
    unsigned char encabezado[TAMANO_ENCABEZADO];

    memset(encabezado, 0, sizeof(encabezado));
    memcpy(encabezado, MAGIC_PAQUETE, MAGIC_SIZE);

    if (write(descriptor, encabezado, sizeof(encabezado)) !=
        (ssize_t)sizeof(encabezado)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo escribir el encabezado del paquete");
        close(descriptor);
        liberarListaArchivos(&lista);

        return 0;
    }

    /* Vaciar la salida antes del fork para que los hijos no hereden
       texto pendiente en el buffer y lo impriman duplicado. */
    fflush(NULL);

    for (int k = 0; k < trabajadores; k++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error: fork fallo");

            break;
        }

        if (pid == 0) {
            trabajarCompresion(k, trabajadores, &lista, descriptor,
                               compartido, indiceCompartido);
            _exit(0);
        }
    }

    /* Esperar a TODOS los hijos antes de leer los acumuladores */
    while (wait(NULL) > 0) {
        /* wait() devuelve -1 cuando ya no quedan hijos */
    }

    est->archivosProcesados = lista.cantidad;
    est->archivosCorrectos = compartido->archivosCorrectos;
    est->bytesOriginales = compartido->bytesOriginales;

    /* Armar el indice en el orden original de los archivos, saltando los
       que no se pudieron comprimir */
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

    close(descriptor);

    /* El padre escribe el indice y cierra el encabezado */
    FILE *salida = fopen(destino, "r+b");

    if (salida == NULL || indice.entradas == NULL) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo cerrar el paquete");
    }
    else if (fseek(salida, 0, SEEK_END) != 0) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo posicionar al final del paquete");
    }
    else {
        long offsetIndice = ftell(salida);

        if (0 <= offsetIndice && escribirIndice(salida, &indice) &&
            fseek(salida, 0, SEEK_SET) == 0 &&
            escribirEncabezado(salida, indice.cantidad, (uint64_t)offsetIndice)) {

            fflush(salida);
            est->bytesComprimidos = (size_t)offsetIndice;

            if (fseek(salida, 0, SEEK_END) == 0) {
                long total = ftell(salida);

                if (0 <= total) {
                    est->bytesComprimidos = (size_t)total;
                }
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

    if (salida != NULL) {
        fclose(salida);
    }

    if (!est->ok && est->mensaje[0] == '\0') {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "%d de %d archivos no se pudieron comprimir",
                 est->archivosProcesados - est->archivosCorrectos,
                 est->archivosProcesados);
    }

    free(indice.entradas);
    munmap(compartido, sizeof(Compartido));
    munmap(indiceCompartido, sizeof(EntradaIndice) * (size_t)lista.cantidad);
    liberarListaArchivos(&lista);

    return est->ok;
}

/* ---------------------------------------------------------------
   Descompresion
   --------------------------------------------------------------- */

static void trabajarDescompresion(int k, int trabajadores, const Indice *indice,
                                  const char *rutaPaquete, const char *dirSalida,
                                  Compartido *compartido)
{
    /* Cada hijo abre el paquete por su cuenta: asi cada uno tiene su
       propia posicion de lectura y no interfiere con los demas. */
    FILE *entrada = fopen(rutaPaquete, "rb");

    if (entrada == NULL) {
        return;
    }

    int correctos = 0;
    int firmas = 0;
    size_t originales = 0;
    size_t comprimidos = 0;

    for (uint32_t i = (uint32_t)k; i < indice->cantidad;
         i += (uint32_t)trabajadores) {

        const EntradaIndice *meta = &indice->entradas[i];
        char ruta[4096];

        if ((int)sizeof(ruta) <=
            snprintf(ruta, sizeof(ruta), "%s/%s", dirSalida, meta->nombre)) {
            continue;
        }

        int coincide = 0;

        /* extraerArchivo hace fseek al offset que trae el indice, asi que
           cada hijo llega directo a su archivo sin leer los anteriores. */
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
}

int descomprimirParalelo(const char *origen, const char *destino, Estadisticas *est)
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

    Compartido *compartido = reservarCompartida(sizeof(Compartido));

    if (compartido == NULL || !iniciarCompartido(compartido)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo reservar la memoria compartida");
        liberarIndice(&indice);

        return 0;
    }

    fflush(NULL);

    for (int k = 0; k < trabajadores; k++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error: fork fallo");

            break;
        }

        if (pid == 0) {
            trabajarDescompresion(k, trabajadores, &indice, origen, destino,
                                  compartido);
            _exit(0);
        }
    }

    while (wait(NULL) > 0) {
        /* esperar a todos los hijos */
    }

    est->segundos = relojSegundos() - inicio;
    est->archivosProcesados = (int)indice.cantidad;
    est->archivosCorrectos = compartido->archivosCorrectos;
    est->firmasVerificadas = compartido->firmasVerificadas;
    est->bytesOriginales = compartido->bytesOriginales;
    est->bytesComprimidos = compartido->bytesComprimidos;
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

    munmap(compartido, sizeof(Compartido));
    liberarIndice(&indice);

    return est->ok;
}