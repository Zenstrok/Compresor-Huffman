#include <stdio.h>
#include <openssl/evp.h>

#include "md5.h"

int calcularMD5(const char *ruta, unsigned char firma[MD5_SIZE])
{
    FILE *archivo = fopen(ruta, "rb");

    if (archivo == NULL) {
        perror("Error de MD5: No se pudo abrir el archivo");

        return 0;
    }

    /* Contexto de OpenSSL para calcular el hash por bloques, sin cargar
       todo el archivo a memoria */
    EVP_MD_CTX *contexto = EVP_MD_CTX_new();

    if (contexto == NULL) {
        fclose(archivo);

        return 0;
    }

    /* Indicamos que el algoritmo es MD5 */
    if (EVP_DigestInit_ex(contexto, EVP_md5(), NULL) != 1) {
        EVP_MD_CTX_free(contexto);
        fclose(archivo);

        return 0;
    }

    unsigned char buffer[4096];
    size_t leidos;

    while ((leidos = fread(buffer, 1, sizeof(buffer), archivo)) > 0) {
        if (EVP_DigestUpdate(contexto, buffer, leidos) != 1) {
            EVP_MD_CTX_free(contexto);
            fclose(archivo);

            return 0;
        }
    }

    if (ferror(archivo)) {
        perror("Error de MD5: Error leyendo el archivo");
        EVP_MD_CTX_free(contexto);
        fclose(archivo);

        return 0;
    }

    unsigned int longitudFirma = 0;

    if (EVP_DigestFinal_ex(contexto, firma, &longitudFirma) != 1 ||
        longitudFirma != MD5_SIZE) {
        EVP_MD_CTX_free(contexto);
        fclose(archivo);

        return 0;
    }

    EVP_MD_CTX_free(contexto);
    fclose(archivo);

    return 1;
}

void imprimirMD5(const unsigned char firma[MD5_SIZE])
{
    for (int i = 0; i < MD5_SIZE; i++) {
        printf("%02x", firma[i]);
    }

    printf("\n");
}
