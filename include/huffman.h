#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stddef.h>

/* Nodo del arbol de Huffman. Tambien se usa como nodo de la lista
   enlazada de frecuencias mientras se construye el arbol. */
struct nodo {
    char caracter;
    int total;
    int bit;

    struct nodo *siguiente;
    struct nodo *izquierda;
    struct nodo *derecha;
};

/* Entrada del diccionario: un caracter y su codigo de Huffman escrito
   en ASCII ('0' y '1'). El maximo teorico de un codigo es 256 bits. */
struct diccionario {
    char caracter;
    char bit[257];
    struct diccionario *siguiente;
};

/* Si vale 1, huffman() imprime el arbol y el diccionario.
   Debe quedar en 0 al medir tiempos. Definida en huffman.c. */
extern int huffmanVerboso;

/* Construye el arbol de Huffman a partir de los datos y devuelve la raiz.
   Deja en *diccionarioSalida la lista de codigos, que el llamador debe
   liberar con liberarDiccionario(). Devuelve NULL si largo es 0. */
struct nodo *huffman(const unsigned char texto[], size_t largo,
                     struct diccionario **diccionarioSalida);

/* Devuelve el codigo del caracter, o NULL si no esta en el diccionario. */
const char *buscarCodigo(const struct diccionario *inicio, unsigned char caracter);

void liberarDiccionario(struct diccionario *inicio);
void liberarArbol(struct nodo *raiz);
void recorrerArbol(struct nodo *raiz, int nivel);

#endif
