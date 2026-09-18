#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compresor.h"
#include "archivo.h"
#include "directorio.h"
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

int agregarArchivoAlPaquete(FILE *salida, const char *rutaOriginal,
                            EntradaIndice *entrada)
{
    unsigned char *datos = NULL;
    size_t tamano = 0;

    memset(entrada, 0, sizeof(EntradaIndice));

    /* Nombre del archivo, sin su directorio */
    const char *base = strrchr(rutaOriginal, '/');

    base = base != NULL ? base + 1 : rutaOriginal;

    if (MAX_NOMBRE <= strlen(base)) {
        fprintf(stderr, "Error: nombre demasiado largo: %s\n", base);

        return 0;
    }

    strcpy(entrada->nombre, base);

    /* 1) Calcular el MD5 ANTES de comprimir */
    if (!calcularMD5(rutaOriginal, entrada->firma)) {
        return 0;
    }

    /* 2) Leer el archivo */
    if (!cargarArchivo(rutaOriginal, &datos, &tamano)) {
        return 0;
    }

    entrada->tamanoOriginal = (uint64_t)tamano;

    /* 3) Construir el arbol */
    struct diccionario *diccionario = NULL;
    struct nodo *raiz = huffman(datos, tamano, &diccionario);

    int resultado = 0;
    long offsetBloque = ftell(salida);

    if (offsetBloque < 0) {
        goto limpiar;
    }

    if (0 < tamano && raiz == NULL) {
        goto limpiar;
    }

    entrada->offsetBloque = (uint64_t)offsetBloque;

    /* 4) Escribir el bloque: marca de arbol, arbol y flujo de bits */
    unsigned char tieneArbol = raiz != NULL ? 1 : 0;

    if (fwrite(&tieneArbol, 1, 1, salida) != 1) {
        goto limpiar;
    }

    if (raiz != NULL && !escribirArbol(salida, raiz)) {
        goto limpiar;
    }

    if (!escribirDatosComprimidos(salida, diccionario, datos, tamano)) {
        goto limpiar;
    }

    long offsetFinal = ftell(salida);

    if (offsetFinal < 0) {
        goto limpiar;
    }

    entrada->tamanoBloque = (uint64_t)(offsetFinal - offsetBloque);
    resultado = 1;

limpiar:
    free(datos);
    liberarDiccionario(diccionario);
    liberarArbol(raiz);

    return resultado;
}

int comprimirDirectorio(const char *dirEntrada, const char *rutaPaquete,
                        Estadisticas *est)
{
    estadisticasIniciar(est);

    ListaArchivos lista;

    if (!listarArchivos(dirEntrada, ".txt", &lista)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo leer el directorio: %s", dirEntrada);

        return 0;
    }

    if (lista.cantidad == 0) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se encontraron archivos .txt en: %s", dirEntrada);
        liberarListaArchivos(&lista);

        return 0;
    }

    FILE *salida = fopen(rutaPaquete, "wb");

    if (salida == NULL) {
        perror("Error: no se pudo crear el paquete comprimido");
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo crear el paquete: %s", rutaPaquete);
        liberarListaArchivos(&lista);

        return 0;
    }

    Indice indice;

    indice.cantidad = 0;
    indice.entradas = calloc((size_t)lista.cantidad, sizeof(EntradaIndice));

    if (indice.entradas == NULL) {
        fclose(salida);
        liberarListaArchivos(&lista);

        return 0;
    }

    double inicio = relojSegundos();

    /* Encabezado provisional: al terminar se reescribe con los datos reales */
    if (!escribirEncabezado(salida, 0, 0)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo escribir el encabezado del paquete");
        goto limpiar;
    }

    for (int i = 0; i < lista.cantidad; i++) {
        est->archivosProcesados++;

        EntradaIndice entrada;

        if (!agregarArchivoAlPaquete(salida, lista.rutas[i], &entrada)) {
            fprintf(stderr, "Aviso: se omitio %s\n", lista.rutas[i]);

            continue;
        }

        indice.entradas[indice.cantidad] = entrada;
        indice.cantidad++;

        est->archivosCorrectos++;
        est->bytesOriginales += (size_t)entrada.tamanoOriginal;
    }

    /* El indice va al final del paquete */
    long offsetIndice = ftell(salida);

    if (offsetIndice < 0 || !escribirIndice(salida, &indice)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo escribir el indice del paquete");
        goto limpiar;
    }

    est->bytesComprimidos = (size_t)ftell(salida);

    /* Volver al inicio y escribir el encabezado definitivo */
    if (fseek(salida, 0, SEEK_SET) != 0 ||
        !escribirEncabezado(salida, indice.cantidad, (uint64_t)offsetIndice)) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "No se pudo cerrar el encabezado del paquete");
        goto limpiar;
    }

    est->segundos = relojSegundos() - inicio;
    est->ok = 0 < est->archivosCorrectos &&
              est->archivosProcesados == est->archivosCorrectos;

    if (!est->ok) {
        snprintf(est->mensaje, sizeof(est->mensaje),
                 "%d de %d archivos no se pudieron comprimir",
                 est->archivosProcesados - est->archivosCorrectos,
                 est->archivosProcesados);
    }

limpiar:
    fclose(salida);
    free(indice.entradas);
    liberarListaArchivos(&lista);

    return est->ok;
}
