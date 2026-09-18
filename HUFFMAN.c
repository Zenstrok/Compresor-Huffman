#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/evp.h>

//MD5 siempre genera como resultado un hash de 16 bytes, no importar el size del archivo
#define MD5_SIZE 16
#define MAGIC_SIZE 6 // Tamano para el identificador del formato huff(Archivo comprimido) 

    const unsigned char MAGIC_HUFF[MAGIC_SIZE] = {'H', 'U', 'F', 'F', 'V', '1'}; // HUFFV1
    struct nodo{
        char caracter;
        int total;
        int bit;
        
        struct nodo *siguiente;
        struct nodo *izquierda;
        struct nodo *derecha;
        
    };
    struct diccionario{
        char caracter;
        char bit[257];
        struct diccionario *siguiente;
    
    
    };
    void recorrerArbol(struct nodo *raiz, int nivel){
        if(raiz == NULL){
            return;
        }

        for(int i = 0; i < nivel; i++){
            printf("    ");
        }

        if(raiz->caracter == '\0'){
            printf("[Padre | peso: %d | bit: %d]\n",
                raiz->total,
                raiz->bit);
        }
        else if(raiz->caracter == ' '){
            printf("[ESPACIO | peso: %d | bit: %d]\n",
                raiz->total,
                raiz->bit);
        }
        else{
            printf("[%c | peso: %d | bit: %d]\n",
                raiz->caracter,
                raiz->total,
                raiz->bit);
        }

        recorrerArbol(raiz->izquierda, nivel + 1);
        recorrerArbol(raiz->derecha, nivel + 1);
    }


    void construirDiccionario(struct nodo *punteroActual, char bits[],
                          struct diccionario **inicio, struct diccionario **ultimo)
    {
        if (punteroActual == NULL) {
            return;
        }

        // Es una hoja
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
                // Un solo caracter distinto: el arbol es solo la raiz
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


    struct nodo *huffman(const unsigned char texto[], size_t largo,
                     struct diccionario **diccionarioSalida) {
        *diccionarioSalida = NULL;

        struct nodo *inicio = NULL;        
        
        for(size_t i = 0; i< largo ; i++){
            char caracter_actual = texto[i];
    
            if(inicio==NULL){
                //primer nodo
                struct nodo *nuevo = malloc(sizeof(struct nodo));
                nuevo->caracter = caracter_actual;
                nuevo->total= 1;
                nuevo->bit = -1;
                nuevo->siguiente=NULL;
                nuevo->izquierda = NULL;
                nuevo->derecha = NULL;
                inicio = nuevo;
                   
            }
            
            else{
                //actual sirve para moverme
                struct nodo *actual = inicio;
                //pregunto por el primero, que es caso aparte en el while
                if(actual->caracter==caracter_actual){
                    actual->total = actual->total + 1;
                }
                else{
                    //bandera para saber si ya estaba el char
                    int bandera = 0;
                    while(actual->siguiente != NULL){
                        //si ya estaba
                        if(actual->caracter==caracter_actual){
                            actual->total = actual->total + 1;
                            bandera = 1; //aviso que ya estaba
                            break;
                        }
                        //siga
                        actual = actual->siguiente;
                        
                    }
                    //si es el ultimo y ya estaba
                    if(bandera == 0 && actual->caracter==caracter_actual){
                            actual->total = actual->total + 1;
                            bandera = 1;
                        }
                    //si no estaba
                    if(bandera==0){
                        struct nodo *nuevo = malloc(sizeof(struct nodo));
                        nuevo->caracter =  caracter_actual;
                        nuevo->total= 1;
                        nuevo->bit = -1;
                        nuevo->siguiente=NULL;
                        nuevo->izquierda = NULL;
                        nuevo->derecha = NULL;
                        actual->siguiente = nuevo;
                    }
                }
                 
                
            }
        }
    
        
        
        //para ordenar creando copia
        struct nodo *nuevoInicio = NULL;
        struct nodo *copiaInicio = inicio;
        while(copiaInicio != NULL){
            //si en la lista nueva ordenada no hay nada, lo ponemos ahi
            if(nuevoInicio==NULL){
                struct nodo *nuevo = malloc(sizeof(struct nodo));
                nuevo->caracter =  copiaInicio->caracter;
                nuevo->total= copiaInicio->total;
                nuevo->bit = -1;
                nuevo->siguiente=NULL;
                nuevo->izquierda = NULL;
                nuevo->derecha = NULL;
                nuevoInicio = nuevo;
            }
            else{
                //si ya habia algo, pregunto si el nuevo tiene mayor peso y lo pongo adelante
                if(copiaInicio->total<nuevoInicio->total){
                    struct nodo *nuevo = malloc(sizeof(struct nodo));
                    nuevo->caracter =  copiaInicio->caracter;
                    nuevo->total= copiaInicio->total;
                    nuevo->bit = -1;
                    nuevo->siguiente=nuevoInicio;
                    nuevo->izquierda = NULL;
                    nuevo->derecha = NULL;
                    nuevoInicio = nuevo;
                }
                //lo pongo a la derecha 
                else{
                    struct nodo *nuevo = malloc(sizeof(struct nodo));
                
                    nuevo->caracter = copiaInicio->caracter;
                    nuevo->total = copiaInicio->total;
                    nuevo->bit = -1;
                    nuevo->izquierda = NULL;
                    nuevo->derecha = NULL;
                    nuevo->siguiente = NULL;
                
                    struct nodo *iterar = nuevoInicio;
                
                    while(iterar->siguiente != NULL &&
                          iterar->siguiente->total <= nuevo->total){
                        iterar = iterar->siguiente;
                    }
                
                    nuevo->siguiente = iterar->siguiente;
                    iterar->siguiente = nuevo;
                }
            }
            copiaInicio = copiaInicio->siguiente;
        }
        //Recorrer la lista
        struct nodo *prueba = nuevoInicio;
        while(prueba!=NULL){
            printf("Carácter: %c \n", prueba->caracter);
            printf("Cantidad: %d \n", prueba->total);
            printf("Cantidad: %s \n", "----------------");
            prueba = prueba->siguiente;
        }
        //borrar el primero
        struct nodo *temporal;
    
        while(inicio != NULL){
            temporal = inicio;
            inicio = inicio->siguiente;
            free(temporal);
        }

        // Si el archivo estaba vacio no hay lista ni arbol que construir
        if (nuevoInicio == NULL) {
            return NULL;
        }
        
        // Crear el árbol de Huffman
        while(nuevoInicio->siguiente != NULL){
        
            // Los dos primeros son los de menor peso
            struct nodo *primero = nuevoInicio;
            struct nodo *segundo = nuevoInicio->siguiente;
        
            // Quitarlos de la lista
            nuevoInicio = segundo->siguiente;
        
            // Crear el nodo padre
            struct nodo *padre = malloc(sizeof(struct nodo));
        
            padre->caracter = '\0';
            padre->total = primero->total + segundo->total;
            padre->bit = -1;


            padre->izquierda = primero;
            padre->derecha = segundo;
            primero->bit = 0;
            segundo->bit = 1;
        
            // El padre se va a insertar en la lista
            padre->siguiente = NULL;
        
            // Insertarlo manteniendo el orden
            if(nuevoInicio == NULL || padre->total < nuevoInicio->total){
                padre->siguiente = nuevoInicio;
                nuevoInicio = padre;
            }
            else{
                struct nodo *actual = nuevoInicio;
        
                while(actual->siguiente != NULL &&
                      actual->siguiente->total <= padre->total){
                    actual = actual->siguiente;
                }
        
                padre->siguiente = actual->siguiente;
                actual->siguiente = padre;
            }
        }
        recorrerArbol(nuevoInicio, 0);

        char bits[257] = "";
        struct diccionario *ultimo = NULL;

        construirDiccionario(nuevoInicio, bits, diccionarioSalida, &ultimo);

        // Imprimir diccionario
        struct diccionario *prueba2 = *diccionarioSalida;

        while (prueba2 != NULL) {
            printf("Carácter: %c \n", prueba2->caracter);
            printf("Bits: %s \n", prueba2->bit);
            printf("----------------\n");

            prueba2 = prueba2->siguiente;
        }
    return nuevoInicio;
    } 

    int calcularMD5(const char *ruta, unsigned char firma[MD5_SIZE]) {
        FILE *archivo = fopen(ruta, "rb");

        if (archivo == NULL) {
            perror("Error de MD5: No se pudo abrir el archivo");

            return 0;
        }

        // Estructura de Openssl para guardar el estado actual del calculo del MD5, no se carga todo de una al RAM
        EVP_MD_CTX *contexto = EVP_MD_CTX_new();

        // Si el archivo de texto es nulo
        if (contexto == NULL) {
            fclose(archivo);

            return 0;
        }

        // Indicamos que se utiliza MD5
        if (EVP_DigestInit_ex(contexto, EVP_md5(), NULL) != 1) {
            EVP_MD_CTX_free(contexto);
            fclose(archivo);

            return 0;
        }

        // Buffer para leer de 4KB en 4KB
        unsigned char buffer[4096];
        size_t bytesLeidos; // 64 bits para fread()

        // Leer el archivo por bloques
        while (0 < (bytesLeidos = fread(buffer, 1, sizeof(buffer), archivo))) {
            if (EVP_DigestUpdate(contexto, buffer, bytesLeidos) != 1) {
                EVP_MD_CTX_free(contexto);
                fclose(archivo);
            }
        }

        if (ferror(archivo)) {
            perror("Error de MD5: Error leyendo el archivo");

            EVP_MD_CTX_free(contexto);
            fclose(archivo);
        }

        unsigned int longitudFirma = 0;

        // Generar el MD5 final
        if (EVP_DigestFinal_ex(contexto, firma, &longitudFirma) != 1 || longitudFirma != MD5_SIZE) {

            EVP_MD_CTX_free(contexto);
            fclose(archivo);
        }

        EVP_MD_CTX_free(contexto);
        fclose(archivo);

        return 1;
    }

    void imprimirMD5(const unsigned char firma[MD5_SIZE]) {
        for (int i = 0; i < MD5_SIZE; i++) {
            printf("%02x", firma[i]);
        }
        printf("\n");
    }

    int cargarArchivo(const char *ruta,unsigned char **datos, size_t *tamano) {
        FILE *archivo = fopen(ruta, "rb");

        if (archivo == NULL) {
            perror("Error de cargar: No se pudo abrir el archivo");
            
            return 0;
        }

        if (fseek(archivo, 0, SEEK_END) != 0) {
            fclose(archivo);

            return 0;
        }

        long tamanoArchivo = ftell(archivo);

        // Si el archivo esta vacio, cerrarlo
        if (tamanoArchivo < 0) {
            fclose(archivo);

            return 0;
        }

        rewind(archivo);

        *tamano = (size_t)tamanoArchivo;

        if (*tamano == 0) {
            *datos = NULL;
            fclose(archivo);

            return 1;
        }

        *datos = malloc(*tamano);

        if (*datos == NULL) {
            fclose(archivo);

            return 0;
        }

        size_t leidos = fread(*datos, 1, *tamano, archivo);

        fclose(archivo);

        if (leidos != *tamano) {
            free(*datos);
            *datos = NULL;

            return 0;
        }

        return 1;
    }

    int escribirArbol(FILE *salida, struct nodo *raiz) {
        if (raiz == NULL) {
            return 1;
        }

        // Es una hoja
        if (raiz->izquierda == NULL &&
            raiz->derecha == NULL) {

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

        // Es un nodo padre
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

    int escribirMetadatos(FILE *archivoComprimido, const unsigned char firma[MD5_SIZE], uint64_t tamanoOriginal, struct nodo *raiz) {
        // HUFFV1 (Identificador nuestro para verificar el formato)
        if (fwrite(MAGIC_HUFF, 1, MAGIC_SIZE, archivoComprimido) != MAGIC_SIZE) {
            return 0;
        }

        // MD5
        if (fwrite(firma, 1, MD5_SIZE, archivoComprimido) != MD5_SIZE) {
            return 0;
        }

        // Tamano del archivo original
        if (fwrite(&tamanoOriginal,sizeof(uint64_t), 1, archivoComprimido) != 1) {
            return 0;
        }

        // Indicar si existe el arbol
        unsigned char tieneArbol = raiz != NULL ? 1 : 0;

        if (fwrite(&tieneArbol, 1, 1, archivoComprimido) != 1) {
            return 0;
        }

        // Guardar el arbol
        if (raiz != NULL) {
            if (!escribirArbol(archivoComprimido, raiz)) {
                return 0;
            }
        }

        return 1;
    }

    //Para buscar el codigo de cada caracter
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

    int escribirDatosComprimidos(FILE *salida, const struct diccionario *diccionario,
                             const unsigned char *datos, size_t tamano) {
        unsigned char byteActual = 0;
        int bitsUsados = 0;

        for (size_t i = 0; i < tamano; i++) {
            const char *codigo = buscarCodigo(diccionario, datos[i]);

            if (codigo == NULL) {
                fprintf(stderr, "Error de escribir datos comprimidos: Caracter sin codigo Huffman.\n");

                return 0;
            }

            for (size_t j = 0; codigo[j] != '\0'; j++ ) {
                // Dejar espacio para el siguiente bit
                byteActual <<= 1;

                // Si el codigo dice 1, poner el último bit en 1
                if (codigo[j] == '1') {
                    byteActual |= 1;
                }

                bitsUsados++;

                // Cuando tenemos 8 bits completos, guardar un byte real
                if (bitsUsados == 8) {
                    if (fwrite(&byteActual, 1, 1, salida) != 1) {

                        return 0;
                    }

                    byteActual = 0;
                    bitsUsados = 0;
                }
            }
        }

        // Si al final quedaron bits incompletos
        if (0 < bitsUsados) {
            // Completar con ceros a la derecha
            byteActual <<= (8 - bitsUsados);

            if (fwrite(&byteActual, 1, 1, salida) != 1) {
                return 0;
            }
        }

        return 1;
    }

    int comprimirArchivo(const char *rutaOriginal, const char *rutaComprimida) {
        unsigned char firmaOriginal[MD5_SIZE];
        unsigned char *datos = NULL;
        size_t tamano = 0;

        // 1) Calcular MD5 antes de comprimir
        if (!calcularMD5(rutaOriginal, firmaOriginal)) {
            return 0;
        }

        printf("MD5 original: ");
        imprimirMD5(firmaOriginal);

        // 2) Leer archivo
        if (!cargarArchivo(rutaOriginal, &datos, &tamano)) {
            return 0;
        }

        printf("Tamaño original: %zu bytes\n", tamano);

        // 3) Construir arbol
        struct diccionario *diccionario = NULL;
        struct nodo *raiz = huffman(datos, tamano, &diccionario);

        int resultado = 0;
        FILE *salida = NULL;
        long tamanoComprimido = 0;

        if (0 < tamano && raiz == NULL) {
            goto limpiar;
        }

        // 4) Crear el archivo .huff
        salida = fopen(rutaComprimida, "wb");

        if (salida == NULL) {
            perror("Error de creacion de .huff: no se pudo crear el archivo comprimido");

            goto limpiar;
        }

        // 5) Guardar metadatos
        if (!escribirMetadatos(salida, firmaOriginal, (uint64_t)tamano, raiz)) {
            goto limpiar;
        }

        // 6) Comprimir el archivo
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
            printf("Tamano comprimido: %ld bytes\n", tamanoComprimido);
            printf("Archivo comprimido correctamente: %s\n", rutaComprimida);
        }

        return resultado;
    }

int main() {
    //huffman("Huffman es un algoritmo de compresion que permite representar los caracteres de un texto utilizando diferentes cantidades de bits. Los caracteres que aparecen con mayor frecuencia reciben codigos mas pequenos, mientras que los caracteres que aparecen pocas veces reciben codigos mas largos. Para construir el arbol de Huffman primero se cuentan las apariciones de cada caracter y despues se ordenan de menor a mayor frecuencia. En este proyecto estamos implementando un sistema capaz de comprimir documentos de texto utilizando Huffman. El programa debe leer diferentes archivos, analizar su contenido, contar los caracteres y construir un arbol binario. Cada hoja del arbol representa un caracter y cada camino desde la raiz hasta una hoja representa el codigo utilizado para codificar ese caracter. La idea principal es que no todos los caracteres necesitan utilizar ocho bits. Por ejemplo, si una letra aparece muchas veces dentro de un documento, podemos asignarle un codigo corto. Si otra letra aparece muy pocas veces, podemos asignarle un codigo mas largo. De esta manera se reduce la cantidad total de informacion necesaria para almacenar el documento original. El programa tambien debe trabajar correctamente con espacios, numeros, signos de puntuacion y diferentes caracteres que puedan aparecer dentro de los documentos. Por ejemplo, una frase puede contener letras mayusculas, letras minusculas, espacios, comas, puntos, dos puntos, signos de interrogacion y numeros. Todos estos elementos deben ser considerados caracteres independientes dentro de la tabla de frecuencias. Durante la construccion del arbol se toman los dos nodos que tienen menor frecuencia y se crea un nuevo nodo padre cuya frecuencia corresponde a la suma de ambos nodos. El primer nodo se coloca como hijo izquierdo y recibe el bit cero, mientras que el segundo nodo se coloca como hijo derecho y recibe el bit uno. Este proceso se repite hasta que solamente queda un nodo, que representa la raiz del arbol de Huffman. Despues de construir el arbol podemos recorrerlo de manera recursiva para obtener el codigo de cada caracter. Si avanzamos hacia la izquierda agregamos un cero al codigo y si avanzamos hacia la derecha agregamos un uno. Cuando encontramos una hoja, todos los bits acumulados representan el codigo completo de ese caracter. Por ejemplo, el caracter espacio puede tener un codigo diferente al de la letra e. La letra e probablemente aparecera muchas veces en un texto comun, mientras que caracteres como x, z, q o algunos signos de puntuacion pueden aparecer menos veces. Esto provoca que los codigos generados por Huffman tengan diferentes longitudes. La compresion de datos es importante porque permite almacenar informacion utilizando menos espacio. En archivos de texto grandes, una buena distribucion de frecuencias puede producir una reduccion considerable del tamano. Sin embargo, para poder recuperar el documento original necesitamos conservar suficiente informacion para reconstruir los datos correctamente. Tambien es importante verificar que el archivo descomprimido sea exactamente igual al archivo original. Para esto se puede generar una firma MD5 antes de comprimir el archivo y generar nuevamente una firma despues de descomprimirlo. Si ambas firmas coinciden, podemos comprobar que el contenido recuperado corresponde al contenido original. Este proyecto tambien considera diferentes formas de ejecutar la compresion. Una version serial procesa los archivos uno despues de otro. Una version paralela puede utilizar procesos independientes para trabajar con diferentes archivos al mismo tiempo. Una version concurrente puede utilizar varios hilos que comparten memoria y coordinan su trabajo. La sincronizacion es necesaria cuando varios procesos o hilos trabajan al mismo tiempo. Si dos partes del programa intentan modificar la misma informacion simultaneamente pueden producirse errores. Por esta razon es necesario utilizar mecanismos de comunicacion y sincronizacion adecuados para garantizar que cada operacion se realice correctamente. Finalmente, el sistema debe permitir comparar el comportamiento de las diferentes implementaciones. Se pueden medir los tiempos de compresion y descompresion, el tamano original de los archivos, el tamano despues de la compresion, la razon de compresion y la aceleracion obtenida mediante paralelismo o concurrencia. El objetivo final es obtener un programa funcional que permita comprimir y descomprimir documentos, verificar que la informacion no haya cambiado y comparar el rendimiento de las diferentes estrategias implementadas. Huffman proporciona la estructura necesaria para realizar la compresion, mientras que las tecnicas de procesos e hilos permiten estudiar como mejorar el rendimiento cuando existen muchos archivos que deben ser procesados.");

    if (comprimirArchivo("prueba.txt", "prueba.huff")) {
        printf("Compresion completado\n");
    }
    else {
        printf("Error de compresion\n");
    }

    return 0;
}