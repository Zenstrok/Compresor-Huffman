#include "formato.h"

const unsigned char MAGIC_HUFF[MAGIC_SIZE] = { 'H', 'U', 'F', 'F', 'V', '1' };

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

        if (fwrite(&caracter, 1, 1, salida) != 1) {
            return 0;
        }

        return 1;
    }

    /* Es un nodo interno */
    unsigned char marcador = 0;

    if (fwrite(&marcador, 1, 1, salida) != 1) {
        return 0;
    }

    if (!escribirArbol(salida, raiz->izquierda)) {
        return 0;
    }

    if (!escribirArbol(salida, raiz->derecha)) {
        return 0;
    }

    return 1;
}

int escribirMetadatos(FILE *archivoComprimido, const unsigned char firma[MD5_SIZE],
                      uint64_t tamanoOriginal, struct nodo *raiz)
{
    /* Identificador del formato */
    if (fwrite(MAGIC_HUFF, 1, MAGIC_SIZE, archivoComprimido) != MAGIC_SIZE) {
        return 0;
    }

    /* MD5 del original */
    if (fwrite(firma, 1, MD5_SIZE, archivoComprimido) != MD5_SIZE) {
        return 0;
    }

    /* Tamano del archivo original */
    if (fwrite(&tamanoOriginal, sizeof(uint64_t), 1, archivoComprimido) != 1) {
        return 0;
    }

    /* Indicar si existe el arbol */
    unsigned char tieneArbol = raiz != NULL ? 1 : 0;

    if (fwrite(&tieneArbol, 1, 1, archivoComprimido) != 1) {
        return 0;
    }

    if (raiz != NULL) {
        if (!escribirArbol(archivoComprimido, raiz)) {
            return 0;
        }
    }

    return 1;
}
