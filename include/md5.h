#ifndef MD5_H
#define MD5_H

/* MD5 siempre genera un hash de 16 bytes, sin importar el tamano del archivo. */
#define MD5_SIZE 16

/* Calcula el MD5 del archivo en 'ruta' y lo deja en 'firma'.
   Devuelve 1 si tuvo exito, 0 si hubo error. */
int calcularMD5(const char *ruta, unsigned char firma[MD5_SIZE]);

/* Imprime la firma en hexadecimal seguida de un salto de linea. */
void imprimirMD5(const unsigned char firma[MD5_SIZE]);

#endif
