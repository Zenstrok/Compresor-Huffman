# ============================================================
#  Proyecto 1 — Compresor Huffman
#  IC6600 Principios de Sistemas Operativos
#
#  make          compila los programas de consola
#  make gui      compila ademas la interfaz grafica (necesita GTK 4)
#  make clean    borra todo lo compilado
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Iinclude
LDLIBS  = -lcrypto -lpthread

SRC_DIR   = src
GUI_DIR   = gui
BUILD_DIR = build

# Modulos compartidos por el compresor, el descompresor y la interfaz
COMUNES = $(BUILD_DIR)/huffman.o \
          $(BUILD_DIR)/md5.o \
          $(BUILD_DIR)/archivo.o \
          $(BUILD_DIR)/formato.o \
          $(BUILD_DIR)/directorio.o \
          $(BUILD_DIR)/estadisticas.o \
          $(BUILD_DIR)/compresor.o \
          $(BUILD_DIR)/descompresor.o \
          $(BUILD_DIR)/serial.o \
          $(BUILD_DIR)/paralelo.o \
          $(BUILD_DIR)/concurrente.o

COMPRIMIR_OBJ    = $(COMUNES) $(BUILD_DIR)/main_comprimir.o
DESCOMPRIMIR_OBJ = $(COMUNES) $(BUILD_DIR)/main_descomprimir.o
GUI_OBJ          = $(COMUNES) $(BUILD_DIR)/main_gui.o

GTK_CFLAGS = $(shell pkg-config --cflags gtk4 2>/dev/null)
GTK_LIBS   = $(shell pkg-config --libs gtk4 2>/dev/null)

all: comprimir descomprimir

comprimir: $(COMPRIMIR_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

descomprimir: $(DESCOMPRIMIR_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

gui: comprimir-gui

comprimir-gui: $(GUI_OBJ)
	@if [ -z "$(GTK_LIBS)" ]; then \
		echo "ERROR: no se encontro GTK 4. Instale: sudo apt install libgtk-4-dev"; \
		exit 1; \
	fi
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) $(GTK_LIBS)

$(BUILD_DIR)/main_gui.o: $(GUI_DIR)/main_gui.c | $(BUILD_DIR)
	@if [ -z "$(GTK_CFLAGS)" ]; then \
		echo "ERROR: no se encontro GTK 4. Instale: sudo apt install libgtk-4-dev"; \
		exit 1; \
	fi
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) comprimir descomprimir comprimir-gui

.PHONY: all gui clean
