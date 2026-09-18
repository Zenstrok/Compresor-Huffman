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

## Uso desde consola

```bash
./comprimir    <serial|fork|hilos> <dir_entrada> <dir_salida>
./descomprimir <serial|fork|hilos> <dir_entrada> <dir_salida>
```

Ejemplo:

```bash
./comprimir serial pruebas/libros pruebas/comprimidos
./descomprimir serial pruebas/comprimidos pruebas/restaurados
```

La bandera `-v` al final del comando de compresión imprime el árbol y el
diccionario. Sirve para depurar y **no debe usarse al medir tiempos**.

## Uso de la interfaz gráfica

```bash
./comprimir-gui
```

Escoja el directorio con los archivos `.txt`, presione el botón y la tabla
muestra las estadísticas de las seis corridas. Los resultados quedan en
`resultados/<versión>/comprimidos` y `resultados/<versión>/descomprimidos`.

## Estructura

```
include/     interfaces públicas de cada módulo (.h)
src/         implementaciones (.c)
gui/         interfaz gráfica en GTK 4
pruebas/     archivos de ejemplo
build/       objetos intermedios (se genera solo)
```

### Módulos

| Módulo | Estado | Responsable |
|---|---|---|
| `huffman` — árbol y diccionario | Listo | Kendall / Tzu |
| `md5` — firmas | Listo | Tzu |
| `archivo` — lectura a memoria | Listo | Tzu |
| `formato` — escritura del `.huff` | Listo | Tzu |
| `formato` — lectura del `.huff` | Pendiente | Mario |
| `directorio` — recorrido de carpetas | Listo | — |
| `estadisticas` — métricas y reloj | Listo | — |
| `compresor` — compresión de un archivo | Listo | Tzu |
| `descompresor` — descompresión de un archivo | **Pendiente** | Mario |
| `serial` — corrida secuencial | Listo | — |
| `paralelo` — `fork()` + IPC | **Pendiente** | Kendall |
| `concurrente` — `pthread` | **Pendiente** | — |
| `gui` — interfaz gráfica | Esqueleto funcional | — |

Cada archivo pendiente trae en su encabezado el esquema de lo que debe hacer.

## Formato del archivo `.huff`

Documentado en `include/formato.h`.
