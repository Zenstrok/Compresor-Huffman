# Proyecto 1 — Compresor Huffman

IC6600 Principios de Sistemas Operativos — Instituto Tecnológico de Costa Rica

Comprime y descomprime directorios de archivos de texto usando el algoritmo de
Huffman, en tres versiones: serial, paralela con `fork()` e IPC, y concurrente
con `pthread` y memoria compartida. Incluye verificación de integridad con MD5
y una interfaz gráfica que compara las tres versiones.

## Dependencias

```bash
sudo apt update
sudo apt install build-essential libssl-dev libgtk-4-dev
```

- `build-essential` — gcc y make
- `libssl-dev` — encabezados de OpenSSL, necesarios para el MD5
- `libgtk-4-dev` — solo si se va a compilar la interfaz gráfica

## Compilación

```bash
make        # programas de consola: comprimir y descomprimir
make gui    # además la interfaz gráfica: comprimir-gui
make clean  # borra todo lo compilado
```

Más Fácil:
```bash
make clean && make && make gui
```

## Uso desde consola

```bash
./comprimir    <serial|fork|hilos> <dir_entrada> <paquete.huff>
./descomprimir <serial|fork|hilos> <paquete.huff> <dir_salida>
```

Ejemplo:

```bash
./comprimir serial ./libros paquete.huff
./descomprimir serial paquete.huff resultados/serial/descomprimidos
```

Todos los archivos del directorio quedan dentro de un unico `.huff`, con la
firma MD5 de cada uno guardada en el indice del paquete.

La bandera `-v` al final del comando de compresión imprime el árbol y el
diccionario. Sirve para depurar y **no debe usarse al medir tiempos**.

## Uso de la interfaz gráfica

```bash
./comprimir-gui
```

Escoja el directorio con los archivos `.txt` y presione el botón de comprimir;
después el de descomprimir. Los resultados quedan en
`resultados/<versión>/paquete.huff` y `resultados/<versión>/descomprimidos`.

## Estructura

```
include/     interfaces públicas de cada módulo (.h)
src/         implementaciones (.c)
gui/         interfaz gráfica en GTK 4
pruebas/     archivos de ejemplo
build/       objetos intermedios (se genera solo)
```

### Módulos

| Módulo | Estado |
|---|---|
| `huffman` — árbol y diccionario | Listo |
| `md5` — firmas | Listo |
| `archivo` — lectura a memoria | Listo |
| `formato` — escritura del `.huff` | Listo |
| `formato` — lectura del `.huff` | Pendiente |
| `directorio` — recorrido de carpetas | Listo |
| `estadisticas` — métricas y reloj | Listo |
| `compresor` — compresión de un archivo | Listo |
| `descompresor` — descompresión de un archivo | **Pendiente** |
| `serial` — corrida secuencial | Listo |
| `paralelo` — `fork()` + IPC | **Pendiente** |
| `concurrente` — `pthread` | **Pendiente** |
| `gui` — interfaz gráfica | Esqueleto funcional |

Cada archivo pendiente trae en su encabezado el esquema de lo que debe hacer.

## Formato del paquete `.huff`

Documentado en detalle en `include/formato.h`: encabezado de 20 bytes,
bloques de Huffman uno por archivo, e índice al final con el nombre, el
tamaño original, el MD5 y el offset de cada archivo.

## Nota sobre el corpus

Los 100 libros de Gutenberg no se versionan (ver `.gitignore`). Colóquelos
en `pruebas/libros/` después de clonar.
