#include <stdio.h>
#include <stdlib.h>

#include "compresor.h"
#include "archivo.h"
#include "formato.h"
#include "huffman.h"
#include "md5.h"

int escribirDatosComprimidos(FILE *salida, const struct diccionario *diccionario,
                             const unsigned char *datos, size_t tamano)
{
    unsigned char byteActual = 0;
    int bitsUsados = 0;

    for (size_t i = 0; i < tamano; i++) {
        const char *codigo = buscarCodigo(diccionario, datos[i]);

        if (codigo == NULL) {
            fprintf(stderr, "Error al comprimir: caracter sin codigo de Huffman.\n");

            return 0;
        }

        for (size_t j = 0; codigo[j] != '\0'; j++) {
            /* Dejar espacio para el siguiente bit */
            byteActual <<= 1;

            /* Si el codigo dice 1, poner el ultimo bit en 1 */
            if (codigo[j] == '1') {
                byteActual |= 1;
            }

            bitsUsados++;

            /* Cuando hay 8 bits completos, escribir un byte real */
            if (bitsUsados == 8) {
                if (fwrite(&byteActual, 1, 1, salida) != 1) {
                    return 0;
                }

                byteActual = 0;
                bitsUsados = 0;
            }
        }
    }

    /* Si quedaron bits incompletos, rellenar con ceros a la derecha */
    if (0 < bitsUsados) {
        byteActual <<= (8 - bitsUsados);

        if (fwrite(&byteActual, 1, 1, salida) != 1) {
            return 0;
        }
    }

    return 1;
}

int comprimirArchivo(const char *rutaOriginal, const char *rutaComprimida)
{
    unsigned char firmaOriginal[MD5_SIZE];
    unsigned char *datos = NULL;
    size_t tamano = 0;

    /* 1) Calcular el MD5 antes de comprimir */
    if (!calcularMD5(rutaOriginal, firmaOriginal)) {
        return 0;
    }

    /* 2) Leer el archivo */
    if (!cargarArchivo(rutaOriginal, &datos, &tamano)) {
        return 0;
    }

    /* 3) Construir el arbol */
    struct diccionario *diccionario = NULL;
    struct nodo *raiz = huffman(datos, tamano, &diccionario);

    int resultado = 0;
    FILE *salida = NULL;
    long tamanoComprimido = 0;

    if (0 < tamano && raiz == NULL) {
        goto limpiar;
    }

    /* 4) Crear el archivo .huff */
    salida = fopen(rutaComprimida, "wb");

    if (salida == NULL) {
        perror("Error de creacion de .huff: no se pudo crear el archivo comprimido");

        goto limpiar;
    }

    /* 5) Escribir los metadatos */
    if (!escribirMetadatos(salida, firmaOriginal, (uint64_t)tamano, raiz)) {
        goto limpiar;
    }

    /* 6) Escribir los datos comprimidos */
    if (!escribirDatosComprimidos(salida, diccionario, datos, tamano)) {
        goto limpiar;
    }

    tamanoComprimido = ftell(salida);
    resultado = 1;

limpiar:
    if (salida != NULL) {
        fclose(salida);
    }

    free(datos);
    liberarDiccionario(diccionario);
    liberarArbol(raiz);

    if (resultado) {
        printf("MD5 original    : ");
        imprimirMD5(firmaOriginal);
        printf("Tamano original : %zu bytes\n", tamano);
        printf("Tamano .huff    : %ld bytes\n", tamanoComprimido);

        if (0 < tamano) {
            printf("Razon           : %.3f (%.1f %% de reduccion)\n",
                   (double)tamanoComprimido / (double)tamano,
                   100.0 * (1.0 - (double)tamanoComprimido / (double)tamano));
        }

        printf("Comprimido en   : %s\n", rutaComprimida);
    }

    return resultado;
}
