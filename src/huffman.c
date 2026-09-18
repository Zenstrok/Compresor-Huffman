#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "huffman.h"

int huffmanVerboso = 0;

void recorrerArbol(struct nodo *raiz, int nivel)
{
    if (raiz == NULL) {
        return;
    }

    for (int i = 0; i < nivel; i++) {
        printf("    ");
    }

    if (raiz->izquierda != NULL || raiz->derecha != NULL) {
        printf("[Padre | peso: %d | bit: %d]\n", raiz->total, raiz->bit);
    }
    else if (raiz->caracter == ' ') {
        printf("[ESPACIO | peso: %d | bit: %d]\n", raiz->total, raiz->bit);
    }
    else {
        printf("[%c | peso: %d | bit: %d]\n", raiz->caracter, raiz->total, raiz->bit);
    }

    recorrerArbol(raiz->izquierda, nivel + 1);
    recorrerArbol(raiz->derecha, nivel + 1);
}

static void construirDiccionario(struct nodo *punteroActual, char bits[],
                                 struct diccionario **inicio,
                                 struct diccionario **ultimo)
{
    if (punteroActual == NULL) {
        return;
    }

    /* Es una hoja */
    if (punteroActual->izquierda == NULL && punteroActual->derecha == NULL) {
        struct diccionario *nuevo = malloc(sizeof(struct diccionario));

        if (nuevo == NULL) {
            return;
        }

        nuevo->caracter = punteroActual->caracter;

        if (bits[0] != '\0') {
            strcpy(nuevo->bit, bits);
        }
        else {
            /* Un solo caracter distinto: el arbol es solo la raiz */
            strcpy(nuevo->bit, "0");
        }

        nuevo->siguiente = NULL;

        if (*inicio == NULL) {
            *inicio = nuevo;
        }
        else {
            (*ultimo)->siguiente = nuevo;
        }

        *ultimo = nuevo;

        return;
    }

    strcat(bits, "0");
    construirDiccionario(punteroActual->izquierda, bits, inicio, ultimo);
    bits[strlen(bits) - 1] = '\0';

    strcat(bits, "1");
    construirDiccionario(punteroActual->derecha, bits, inicio, ultimo);
    bits[strlen(bits) - 1] = '\0';
}

const char *buscarCodigo(const struct diccionario *inicio, unsigned char caracter)
{
    const struct diccionario *actual = inicio;

    while (actual != NULL) {
        if ((unsigned char)actual->caracter == caracter) {
            return actual->bit;
        }

        actual = actual->siguiente;
    }

    return NULL;
}

void liberarDiccionario(struct diccionario *inicio)
{
    while (inicio != NULL) {
        struct diccionario *temporal = inicio;
        inicio = inicio->siguiente;
        free(temporal);
    }
}

void liberarArbol(struct nodo *raiz)
{
    if (raiz == NULL) {
        return;
    }

    liberarArbol(raiz->izquierda);
    liberarArbol(raiz->derecha);

    free(raiz);
}

struct nodo *huffman(const unsigned char texto[], size_t largo,
                     struct diccionario **diccionarioSalida)
{
    *diccionarioSalida = NULL;

    struct nodo *inicio = NULL;

    /* ---- Contar frecuencias en una lista enlazada ---- */
    for (size_t i = 0; i < largo; i++) {
        char caracter_actual = texto[i];

        if (inicio == NULL) {
            struct nodo *nuevo = malloc(sizeof(struct nodo));
            nuevo->caracter = caracter_actual;
            nuevo->total = 1;
            nuevo->bit = -1;
            nuevo->siguiente = NULL;
            nuevo->izquierda = NULL;
            nuevo->derecha = NULL;
            inicio = nuevo;
        }
        else {
            struct nodo *actual = inicio;

            /* El primero es caso aparte del while */
            if (actual->caracter == caracter_actual) {
                actual->total = actual->total + 1;
            }
            else {
                int bandera = 0;

                while (actual->siguiente != NULL) {
                    if (actual->caracter == caracter_actual) {
                        actual->total = actual->total + 1;
                        bandera = 1;
                        break;
                    }

                    actual = actual->siguiente;
                }

                /* Si es el ultimo de la lista y ya estaba */
                if (bandera == 0 && actual->caracter == caracter_actual) {
                    actual->total = actual->total + 1;
                    bandera = 1;
                }

                /* Si no estaba en la lista */
                if (bandera == 0) {
                    struct nodo *nuevo = malloc(sizeof(struct nodo));
                    nuevo->caracter = caracter_actual;
                    nuevo->total = 1;
                    nuevo->bit = -1;
                    nuevo->siguiente = NULL;
                    nuevo->izquierda = NULL;
                    nuevo->derecha = NULL;
                    actual->siguiente = nuevo;
                }
            }
        }
    }

    /* ---- Copiar la lista ordenada de menor a mayor frecuencia ---- */
    struct nodo *nuevoInicio = NULL;
    struct nodo *copiaInicio = inicio;

    while (copiaInicio != NULL) {
        struct nodo *nuevo = malloc(sizeof(struct nodo));

        nuevo->caracter = copiaInicio->caracter;
        nuevo->total = copiaInicio->total;
        nuevo->bit = -1;
        nuevo->siguiente = NULL;
        nuevo->izquierda = NULL;
        nuevo->derecha = NULL;

        if (nuevoInicio == NULL || nuevo->total < nuevoInicio->total) {
            nuevo->siguiente = nuevoInicio;
            nuevoInicio = nuevo;
        }
        else {
            struct nodo *iterar = nuevoInicio;

            while (iterar->siguiente != NULL &&
                   iterar->siguiente->total <= nuevo->total) {
                iterar = iterar->siguiente;
            }

            nuevo->siguiente = iterar->siguiente;
            iterar->siguiente = nuevo;
        }

        copiaInicio = copiaInicio->siguiente;
    }

    if (huffmanVerboso) {
        struct nodo *prueba = nuevoInicio;

        while (prueba != NULL) {
            printf("Caracter: %c \n", prueba->caracter);
            printf("Cantidad: %d \n", prueba->total);
            printf("----------------\n");
            prueba = prueba->siguiente;
        }
    }

    /* Liberar la lista original de frecuencias */
    while (inicio != NULL) {
        struct nodo *temporal = inicio;
        inicio = inicio->siguiente;
        free(temporal);
    }

    /* Si el archivo estaba vacio no hay lista ni arbol que construir */
    if (nuevoInicio == NULL) {
        return NULL;
    }

    /* ---- Construir el arbol de Huffman ---- */
    while (nuevoInicio->siguiente != NULL) {
        /* Los dos primeros son los de menor peso */
        struct nodo *primero = nuevoInicio;
        struct nodo *segundo = nuevoInicio->siguiente;

        /* Quitarlos de la lista */
        nuevoInicio = segundo->siguiente;

        struct nodo *padre = malloc(sizeof(struct nodo));

        padre->caracter = '\0';
        padre->total = primero->total + segundo->total;
        padre->bit = -1;
        padre->izquierda = primero;
        padre->derecha = segundo;
        padre->siguiente = NULL;

        primero->bit = 0;
        segundo->bit = 1;

        /* Insertar el padre manteniendo el orden */
        if (nuevoInicio == NULL || padre->total < nuevoInicio->total) {
            padre->siguiente = nuevoInicio;
            nuevoInicio = padre;
        }
        else {
            struct nodo *actual = nuevoInicio;

            while (actual->siguiente != NULL &&
                   actual->siguiente->total <= padre->total) {
                actual = actual->siguiente;
            }

            padre->siguiente = actual->siguiente;
            actual->siguiente = padre;
        }
    }

    /* ---- Generar el diccionario de codigos ---- */
    char bits[257] = "";
    struct diccionario *ultimo = NULL;

    construirDiccionario(nuevoInicio, bits, diccionarioSalida, &ultimo);

    if (huffmanVerboso) {
        recorrerArbol(nuevoInicio, 0);

        struct diccionario *prueba2 = *diccionarioSalida;

        while (prueba2 != NULL) {
            printf("Caracter: %c \n", prueba2->caracter);
            printf("Bits: %s \n", prueba2->bit);
            printf("----------------\n");
            prueba2 = prueba2->siguiente;
        }
    }

    return nuevoInicio;
}
