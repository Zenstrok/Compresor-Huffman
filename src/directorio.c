#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "directorio.h"

static int terminaEn(const char *texto, const char *sufijo)
{
    size_t largoTexto = strlen(texto);
    size_t largoSufijo = strlen(sufijo);

    if (largoTexto < largoSufijo) {
        return 0;
    }

    return strcmp(texto + largoTexto - largoSufijo, sufijo) == 0;
}

int listarArchivos(const char *rutaDirectorio, const char *extension,
                   ListaArchivos *lista)
{
    lista->rutas = NULL;
    lista->cantidad = 0;

    DIR *directorio = opendir(rutaDirectorio);

    if (directorio == NULL) {
        perror("Error de directorio: no se pudo abrir");

        return 0;
    }

    int capacidad = 16;
    lista->rutas = malloc((size_t)capacidad * sizeof(char *));

    if (lista->rutas == NULL) {
        closedir(directorio);

        return 0;
    }

    struct dirent *entrada;

    while ((entrada = readdir(directorio)) != NULL) {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }

        if (extension != NULL && !terminaEn(entrada->d_name, extension)) {
            continue;
        }

        char ruta[4096];

        if ((int)sizeof(ruta) <=
            snprintf(ruta, sizeof(ruta), "%s/%s", rutaDirectorio, entrada->d_name)) {
            continue;
        }

        /* Solo archivos regulares, no subdirectorios */
        struct stat info;

        if (stat(ruta, &info) != 0 || !S_ISREG(info.st_mode)) {
            continue;
        }

        if (capacidad == lista->cantidad) {
            capacidad *= 2;
            char **mayor = realloc(lista->rutas, (size_t)capacidad * sizeof(char *));

            if (mayor == NULL) {
                closedir(directorio);
                liberarListaArchivos(lista);

                return 0;
            }

            lista->rutas = mayor;
        }

        lista->rutas[lista->cantidad] = strdup(ruta);

        if (lista->rutas[lista->cantidad] == NULL) {
            closedir(directorio);
            liberarListaArchivos(lista);

            return 0;
        }

        lista->cantidad++;
    }

    closedir(directorio);

    return 1;
}

void liberarListaArchivos(ListaArchivos *lista)
{
    if (lista->rutas == NULL) {
        return;
    }

    for (int i = 0; i < lista->cantidad; i++) {
        free(lista->rutas[i]);
    }

    free(lista->rutas);

    lista->rutas = NULL;
    lista->cantidad = 0;
}

int asegurarDirectorio(const char *ruta)
{
    char parcial[4096];
    size_t largo = strlen(ruta);
    if (largo == 0 || sizeof(parcial) <= largo)
    {
        return 0;
    }
    strcpy(parcial, ruta);
    /* Crear cada nivel de la ruta, como hace "mkdir -p": si se pide
    resultados/serial/descomprimidos y no existe resultados/, se crean
    los tres niveles en orden. */
    for (size_t i = 1; i <= largo; i++)
    {
        if (parcial[i] != '/' && parcial[i] != '\0')
        {
            continue;
        }
        char original = parcial[i];
        parcial[i] = '\0';
        struct stat info;
        if (stat(parcial, &info) != 0)
        {
            if (mkdir(parcial, 0755) != 0 && errno != EEXIST)
            {
                perror("Error de directorio: no se pudo crear");
                return 0;
            }
        }
        else if (!S_ISDIR(info.st_mode))
        {
            fprintf(stderr,
                    "Error de directorio: %s existe y no es un directorio\n",
                    parcial);
            return 0;
        }
        parcial[i] = original;
    }
    return 1;
}

int rutaDestino(char *destino, size_t tamanoDestino, const char *directorio,
                const char *rutaArchivo, const char *extensionNueva)
{
    /* Quedarse solo con el nombre del archivo, sin su directorio */
    const char *base = strrchr(rutaArchivo, '/');

    base = base != NULL ? base + 1 : rutaArchivo;

    /* Quitar la extension vieja, si tiene */
    char nombre[1024];
    size_t largo = strlen(base);
    const char *punto = strrchr(base, '.');

    if (punto != NULL) {
        largo = (size_t)(punto - base);
    }

    if (sizeof(nombre) <= largo) {
        return 0;
    }

    memcpy(nombre, base, largo);
    nombre[largo] = '\0';

    int escritos = snprintf(destino, tamanoDestino, "%s/%s%s",
                            directorio, nombre, extensionNueva);

    return 0 < escritos && (size_t)escritos < tamanoDestino;
}
