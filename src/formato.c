#include <stdlib.h>
#include <string.h>

#include "formato.h"

const unsigned char MAGIC_PAQUETE[MAGIC_SIZE] = {
    'H', 'U', 'F', 'F', 'P', 'K', 'G', '1'
};

/* ---------------------------------------------------------------
   Enteros en little-endian explicito, para que el formato del archivo
   no dependa de la arquitectura de la maquina.
   --------------------------------------------------------------- */

static int escribirU16(FILE *salida, uint16_t valor)
{
    unsigned char bytes[2];

    for (int i = 0; i < 2; i++) {
        bytes[i] = (unsigned char)(valor >> (8 * i));
    }

    return fwrite(bytes, 1, 2, salida) == 2;
}

static int escribirU32(FILE *salida, uint32_t valor)
{
    unsigned char bytes[4];

    for (int i = 0; i < 4; i++) {
        bytes[i] = (unsigned char)(valor >> (8 * i));
    }

    return fwrite(bytes, 1, 4, salida) == 4;
}

static int escribirU64(FILE *salida, uint64_t valor)
{
    unsigned char bytes[8];

    for (int i = 0; i < 8; i++) {
        bytes[i] = (unsigned char)(valor >> (8 * i));
    }

    return fwrite(bytes, 1, 8, salida) == 8;
}

static int leerU16(FILE *entrada, uint16_t *valor)
{
    unsigned char bytes[2];

    if (fread(bytes, 1, 2, entrada) != 2) {
        return 0;
    }

    *valor = (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8);

    return 1;
}

static int leerU32(FILE *entrada, uint32_t *valor)
{
    unsigned char bytes[4];

    if (fread(bytes, 1, 4, entrada) != 4) {
        return 0;
    }

    *valor = 0;

    for (int i = 0; i < 4; i++) {
        *valor |= (uint32_t)bytes[i] << (8 * i);
    }

    return 1;
}

static int leerU64(FILE *entrada, uint64_t *valor)
{
    unsigned char bytes[8];

    if (fread(bytes, 1, 8, entrada) != 8) {
        return 0;
    }

    *valor = 0;

    for (int i = 0; i < 8; i++) {
        *valor |= (uint64_t)bytes[i] << (8 * i);
    }

    return 1;
}

/* ---------------------------------------------------------------
   Escritura
   --------------------------------------------------------------- */

int escribirEncabezado(FILE *salida, uint32_t cantidad, uint64_t offsetIndice)
{
    if (fwrite(MAGIC_PAQUETE, 1, MAGIC_SIZE, salida) != MAGIC_SIZE) {
        return 0;
    }

    if (!escribirU32(salida, cantidad)) {
        return 0;
    }

    return escribirU64(salida, offsetIndice);
}

int escribirArbol(FILE *salida, struct nodo *raiz)
{
    if (raiz == NULL) {
        return 1;
    }

    /* Es una hoja */
    if (raiz->izquierda == NULL && raiz->derecha == NULL) {
        unsigned char marcador = 1;
        unsigned char caracter = (unsigned char)raiz->caracter;

        if (fwrite(&marcador, 1, 1, salida) != 1) {
            return 0;
        }

        return fwrite(&caracter, 1, 1, salida) == 1;
    }

    /* Es un nodo interno */
    unsigned char marcador = 0;

    if (fwrite(&marcador, 1, 1, salida) != 1) {
        return 0;
    }

    if (!escribirArbol(salida, raiz->izquierda)) {
        return 0;
    }

    return escribirArbol(salida, raiz->derecha);
}

int escribirIndice(FILE *salida, const Indice *indice)
{
    for (uint32_t i = 0; i < indice->cantidad; i++) {
        const EntradaIndice *entrada = &indice->entradas[i];
        size_t largoNombre = strlen(entrada->nombre);

        if (!escribirU16(salida, (uint16_t)largoNombre)) {
            return 0;
        }

        if (fwrite(entrada->nombre, 1, largoNombre, salida) != largoNombre) {
            return 0;
        }

        if (!escribirU64(salida, entrada->tamanoOriginal)) {
            return 0;
        }

        if (fwrite(entrada->firma, 1, MD5_SIZE, salida) != MD5_SIZE) {
            return 0;
        }

        if (!escribirU64(salida, entrada->offsetBloque)) {
            return 0;
        }

        if (!escribirU64(salida, entrada->tamanoBloque)) {
            return 0;
        }
    }

    return 1;
}

/* ---------------------------------------------------------------
   Lectura
   --------------------------------------------------------------- */

int leerEncabezado(FILE *entrada, uint32_t *cantidad, uint64_t *offsetIndice)
{
    unsigned char magic[MAGIC_SIZE];

    if (fseek(entrada, 0, SEEK_SET) != 0) {
        return 0;
    }

    if (fread(magic, 1, MAGIC_SIZE, entrada) != MAGIC_SIZE) {
        return 0;
    }

    if (memcmp(magic, MAGIC_PAQUETE, MAGIC_SIZE) != 0) {
        return 0;
    }

    if (!leerU32(entrada, cantidad)) {
        return 0;
    }

    return leerU64(entrada, offsetIndice);
}

int leerIndice(FILE *entrada, uint32_t cantidad, uint64_t offsetIndice,
               Indice *indice)
{
    indice->entradas = NULL;
    indice->cantidad = 0;

    if (cantidad == 0) {
        return 1;
    }

    if (fseek(entrada, (long)offsetIndice, SEEK_SET) != 0) {
        return 0;
    }

    indice->entradas = calloc(cantidad, sizeof(EntradaIndice));

    if (indice->entradas == NULL) {
        return 0;
    }

    for (uint32_t i = 0; i < cantidad; i++) {
        EntradaIndice *actual = &indice->entradas[i];
        uint16_t largoNombre = 0;

        if (!leerU16(entrada, &largoNombre) || MAX_NOMBRE <= largoNombre) {
            liberarIndice(indice);

            return 0;
        }

        if (fread(actual->nombre, 1, largoNombre, entrada) != largoNombre) {
            liberarIndice(indice);

            return 0;
        }

        actual->nombre[largoNombre] = '\0';

        if (!leerU64(entrada, &actual->tamanoOriginal)) {
            liberarIndice(indice);

            return 0;
        }

        if (fread(actual->firma, 1, MD5_SIZE, entrada) != MD5_SIZE) {
            liberarIndice(indice);

            return 0;
        }

        if (!leerU64(entrada, &actual->offsetBloque) ||
            !leerU64(entrada, &actual->tamanoBloque)) {
            liberarIndice(indice);

            return 0;
        }
    }

    indice->cantidad = cantidad;

    return 1;
}

void liberarIndice(Indice *indice)
{
    free(indice->entradas);

    indice->entradas = NULL;
    indice->cantidad = 0;
}
