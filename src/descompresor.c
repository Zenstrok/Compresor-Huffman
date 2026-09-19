#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "descompresor.h"
#include "directorio.h"
#include "huffman.h"
#include "md5.h"

/* Un nodo es hoja cuando no tiene hijos. */
static int esHoja(const struct nodo *nodo)
{
    return nodo->izquierda == NULL && nodo->derecha == NULL;
}

struct nodo *leerArbol(FILE *entrada)
{
    int marcador = fgetc(entrada);

    if (marcador == EOF) {
        return NULL;
    }

    struct nodo *nodo = calloc(1, sizeof(struct nodo));

    if (nodo == NULL) {
        return NULL;
    }

    nodo->bit = -1;

    /* 0x01 seguido de un byte: es una hoja */
    if (marcador == 1) {
        int caracter = fgetc(entrada);

        if (caracter == EOF) {
            free(nodo);

            return NULL;
        }

        nodo->caracter = (char)(unsigned char)caracter;

        return nodo;
    }

    /* Cualquier valor que no sea 0x00 ni 0x01 significa paquete corrupto */
    if (marcador != 0) {
        free(nodo);

        return NULL;
    }

    /* 0x00: nodo interno. Siguen el hijo izquierdo y luego el derecho,
       en el mismo orden en que los escribio escribirArbol(). */
    nodo->izquierda = leerArbol(entrada);

    if (nodo->izquierda == NULL) {
        free(nodo);

        return NULL;
    }

    nodo->derecha = leerArbol(entrada);

    if (nodo->derecha == NULL) {
        liberarArbol(nodo->izquierda);
        free(nodo);

        return NULL;
    }

    return nodo;
}

int extraerArchivo(FILE *entrada, const EntradaIndice *meta,
                   const char *rutaSalida, int *firmaCoincide)
{
    if (firmaCoincide != NULL) {
        *firmaCoincide = 0;
    }

    /* Saltar directo al bloque de este archivo dentro del paquete */
    if (fseek(entrada, (long)meta->offsetBloque, SEEK_SET) != 0) {
        return 0;
    }

    int tieneArbol = fgetc(entrada);

    if (tieneArbol == EOF) {
        return 0;
    }

    FILE *salida = fopen(rutaSalida, "wb");

    if (salida == NULL) {
        perror("Error al descomprimir: no se pudo crear el archivo de salida");

        return 0;
    }

    /* Todo lo que usa el ciclo de decodificacion se declara antes del
       primer goto, porque no se puede saltar por encima de una
       inicializacion hacia la etiqueta de limpieza. */
    struct nodo *raiz = NULL;
    struct nodo *actual = NULL;
    uint64_t escritos = 0;
    unsigned char buffer[4096];
    size_t leidos = 0;
    int resultado = 0;

    /* El archivo original estaba vacio: no hay arbol ni bits */
    if (tieneArbol == 0) {
        resultado = meta->tamanoOriginal == 0;

        goto cerrar;
    }

    raiz = leerArbol(entrada);

    if (raiz == NULL) {
        goto cerrar;
    }

    /* Caso especial: el archivo original tenia un solo caracter distinto,
       asi que la raiz ES la hoja y no hay camino que recorrer. Sin este
       caso aparte el decodificador se queda parado en la raiz. */
    if (esHoja(raiz)) {
        for (uint64_t i = 0; i < meta->tamanoOriginal; i++) {
            if (fputc((unsigned char)raiz->caracter, salida) == EOF) {
                goto cerrar;
            }
        }

        resultado = 1;

        goto cerrar;
    }

    /* Decodificacion: se recorre el arbol bit a bit desde la raiz.
       0 va a la izquierda y 1 a la derecha; al llegar a una hoja se
       escribe su caracter y se vuelve a la raiz. */
    actual = raiz;

    while (escritos < meta->tamanoOriginal &&
           (leidos = fread(buffer, 1, sizeof(buffer), entrada)) > 0) {

        for (size_t i = 0; i < leidos && escritos < meta->tamanoOriginal; i++) {

            /* Del bit mas significativo al menos significativo */
            for (int k = 7; 0 <= k && escritos < meta->tamanoOriginal; k--) {

                actual = ((buffer[i] >> k) & 1) ? actual->derecha : actual->izquierda;

                if (actual == NULL) {
                    goto cerrar;
                }

                if (esHoja(actual)) {
                    if (fputc((unsigned char)actual->caracter, salida) == EOF) {
                        goto cerrar;
                    }

                    escritos++;
                    actual = raiz;
                }
            }
        }
    }

    /* La condicion de parada es el tamano original, NO el final del
       bloque: el ultimo byte viene relleno con ceros. */
    resultado = escritos == meta->tamanoOriginal;

cerrar:
    fclose(salida);
    liberarArbol(raiz);

    /* Verificar la firma solo si el archivo se reconstruyo completo */
    if (resultado) {
        unsigned char firma[MD5_SIZE];

        if (calcularMD5(rutaSalida, firma) && firmaCoincide != NULL) {
            *firmaCoincide = memcmp(firma, meta->firma, MD5_SIZE) == 0;
        }
    }

    return resultado;
}

int descomprimirPaquete(const char *rutaPaquete, const char *dirSalida,
                        Estadisticas *est)
{
    estadisticasIniciar(est);

    if (!asegurarDirectorio(dirSalida)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo crear el directorio de salida: %s", dirSalida);

        return 0;
    }

    FILE *entrada = fopen(rutaPaquete, "rb");

    if (entrada == NULL) {
        perror("Error al descomprimir: no se pudo abrir el paquete");
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo abrir el paquete: %s", rutaPaquete);

        return 0;
    }

    double inicio = relojSegundos();

    uint32_t cantidad = 0;
    uint64_t offsetIndice = 0;

    if (!leerEncabezado(entrada, &cantidad, &offsetIndice)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "El archivo no es un paquete HUFFPKG1 valido: %s", rutaPaquete);
        fclose(entrada);

        return 0;
    }

    Indice indice;

    if (!leerIndice(entrada, cantidad, offsetIndice, &indice)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo leer el indice del paquete");
        fclose(entrada);

        return 0;
    }

    for (uint32_t i = 0; i < indice.cantidad; i++) {
        const EntradaIndice *meta = &indice.entradas[i];
        char destino[4096];

        if ((int)sizeof(destino) <=
            snprintf(destino, sizeof(destino), "%s/%s", dirSalida, meta->nombre)) {
            continue;
        }

        est->archivosProcesados++;

        int coincide = 0;

        if (extraerArchivo(entrada, meta, destino, &coincide)) {
            est->archivosCorrectos++;
            est->bytesOriginales += (size_t)meta->tamanoOriginal;
            est->bytesComprimidos += (size_t)meta->tamanoBloque;

            if (coincide) {
                est->firmasVerificadas++;
            }
        }
        else {
            fprintf(stderr, "Aviso: no se pudo extraer %s\n", meta->nombre);
        }
    }

    est->segundos = relojSegundos() - inicio;
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

    liberarIndice(&indice);
    fclose(entrada);

    return est->ok;
}