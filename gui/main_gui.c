/* Interfaz grafica del Proyecto 1 — Compresor Huffman
 *
 * Usa GTK 4, que es el toolkit nativo de GNOME en Debian 13.
 * La ventana permite escoger un directorio con archivos .txt, correr las
 * tres versiones del compresor y del descompresor, y comparar los
 * resultados en una tabla.
 *
 * La interfaz se conecta con el resto del programa unicamente a traves de
 * include/estrategias.h, asi que cuando se implementen las versiones
 * paralela y concurrente no hay que tocar este archivo.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "estrategias.h"
#include "estadisticas.h"
#include "directorio.h"

#define CANTIDAD_VERSIONES 3
#define CANTIDAD_COLUMNAS  8

static const char *NOMBRES_VERSION[CANTIDAD_VERSIONES] = {
    "Serial",
    "Paralela (fork)",
    "Concurrente (pthread)"
};

static const char *TITULOS_COLUMNA[CANTIDAD_COLUMNAS + 1] = {
    "Version",
    "Salud (%)",
    "T. compresion (s)",
    "T. descompresion (s)",
    "Acel. compresor (%)",
    "Acel. descompresor (%)",
    "Tam. original (B)",
    "Tam. comprimido (B)",
    "Razon"
};

typedef struct {
    GtkWindow *ventana;
    GtkWidget *entradaDirectorio;
    GtkWidget *botonEjecutar;
    GtkWidget *etiquetaEstado;
    GtkWidget *celdas[CANTIDAD_VERSIONES][CANTIDAD_COLUMNAS];
} Aplicacion;

/* ---------------------------------------------------------------
   Utilidades
   --------------------------------------------------------------- */

static void ponerCelda(Aplicacion *app, int fila, int columna, const char *texto)
{
    gtk_label_set_text(GTK_LABEL(app->celdas[fila][columna]), texto);
}

static void limpiarTabla(Aplicacion *app)
{
    for (int fila = 0; fila < CANTIDAD_VERSIONES; fila++) {
        for (int columna = 0; columna < CANTIDAD_COLUMNAS; columna++) {
            ponerCelda(app, fila, columna, "—");
        }
    }
}

/* Crea "base", "base/sub" y devuelve la ruta completa en destino. */
static int prepararSubdirectorio(char *destino, size_t tamano,
                                 const char *base, const char *sub)
{
    if (!asegurarDirectorio(base)) {
        return 0;
    }

    if ((int)tamano <= snprintf(destino, tamano, "%s/%s", base, sub)) {
        return 0;
    }

    return asegurarDirectorio(destino);
}

/* ---------------------------------------------------------------
   Ejecucion de las seis corridas
   --------------------------------------------------------------- */

static void ejecutarCorridas(Aplicacion *app, const char *directorio)
{
    typedef int (*FuncionCorrida)(const char *, const char *, Estadisticas *);

    FuncionCorrida compresores[CANTIDAD_VERSIONES] = {
        comprimirSerial, comprimirParalelo, comprimirConcurrente
    };

    FuncionCorrida descompresores[CANTIDAD_VERSIONES] = {
        descomprimirSerial, descomprimirParalelo, descomprimirConcurrente
    };

    const char *carpetas[CANTIDAD_VERSIONES] = { "serial", "fork", "hilos" };

    double segundosCompresionSerial = 0.0;
    double segundosDescompresionSerial = 0.0;

    for (int i = 0; i < CANTIDAD_VERSIONES; i++) {
        char dirComprimido[4096];
        char dirDescomprimido[4096];
        char base[4096];

        snprintf(base, sizeof(base), "resultados/%s", carpetas[i]);

        if (!prepararSubdirectorio(dirComprimido, sizeof(dirComprimido),
                                   "resultados", carpetas[i]) ||
            !prepararSubdirectorio(dirComprimido, sizeof(dirComprimido),
                                   base, "comprimidos") ||
            !prepararSubdirectorio(dirDescomprimido, sizeof(dirDescomprimido),
                                   base, "descomprimidos")) {
            ponerCelda(app, i, 0, "error de carpetas");

            continue;
        }

        Estadisticas compresion;
        Estadisticas descompresion;

        compresores[i](directorio, dirComprimido, &compresion);
        descompresores[i](dirComprimido, dirDescomprimido, &descompresion);

        if (i == 0) {
            segundosCompresionSerial = compresion.segundos;
            segundosDescompresionSerial = descompresion.segundos;
        }

        char texto[64];

        snprintf(texto, sizeof(texto), "%.1f", estadisticasSalud(&descompresion));
        ponerCelda(app, i, 0, texto);

        snprintf(texto, sizeof(texto), "%.6f", compresion.segundos);
        ponerCelda(app, i, 1, texto);

        snprintf(texto, sizeof(texto), "%.6f", descompresion.segundos);
        ponerCelda(app, i, 2, texto);

        snprintf(texto, sizeof(texto), "%.1f",
                 estadisticasAceleracion(segundosCompresionSerial, compresion.segundos));
        ponerCelda(app, i, 3, texto);

        snprintf(texto, sizeof(texto), "%.1f",
                 estadisticasAceleracion(segundosDescompresionSerial,
                                         descompresion.segundos));
        ponerCelda(app, i, 4, texto);

        snprintf(texto, sizeof(texto), "%zu", compresion.bytesOriginales);
        ponerCelda(app, i, 5, texto);

        snprintf(texto, sizeof(texto), "%zu", compresion.bytesComprimidos);
        ponerCelda(app, i, 6, texto);

        snprintf(texto, sizeof(texto), "%.3f", estadisticasRazon(&compresion));
        ponerCelda(app, i, 7, texto);

        /* Si alguna de las dos corridas dejo mensaje, mostrarlo */
        if (compresion.mensaje[0] != '\0') {
            gtk_label_set_text(GTK_LABEL(app->etiquetaEstado), compresion.mensaje);
        }
        else if (descompresion.mensaje[0] != '\0') {
            gtk_label_set_text(GTK_LABEL(app->etiquetaEstado), descompresion.mensaje);
        }
    }
}

static void alPresionarEjecutar(GtkButton *boton, gpointer datos)
{
    (void)boton;

    Aplicacion *app = datos;

    const char *directorio = gtk_editable_get_text(GTK_EDITABLE(app->entradaDirectorio));

    if (directorio == NULL || directorio[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(app->etiquetaEstado),
                           "Escoja primero un directorio con archivos .txt.");

        return;
    }

    limpiarTabla(app);
    gtk_label_set_text(GTK_LABEL(app->etiquetaEstado), "Ejecutando las seis corridas...");
    gtk_widget_set_sensitive(app->botonEjecutar, FALSE);

    ejecutarCorridas(app, directorio);

    gtk_widget_set_sensitive(app->botonEjecutar, TRUE);

    const char *estado = gtk_label_get_text(GTK_LABEL(app->etiquetaEstado));

    if (strcmp(estado, "Ejecutando las seis corridas...") == 0) {
        gtk_label_set_text(GTK_LABEL(app->etiquetaEstado),
                           "Listo. Resultados en la carpeta resultados/.");
    }
}

/* ---------------------------------------------------------------
   Selector de directorio
   --------------------------------------------------------------- */

static void alEscogerCarpeta(GObject *fuente, GAsyncResult *resultado, gpointer datos)
{
    Aplicacion *app = datos;
    GtkFileDialog *dialogo = GTK_FILE_DIALOG(fuente);
    GError *error = NULL;

    GFile *carpeta = gtk_file_dialog_select_folder_finish(dialogo, resultado, &error);

    if (carpeta == NULL) {
        /* El usuario cancelo: no es un error que valga la pena mostrar */
        if (error != NULL) {
            g_error_free(error);
        }

        return;
    }

    char *ruta = g_file_get_path(carpeta);

    if (ruta != NULL) {
        gtk_editable_set_text(GTK_EDITABLE(app->entradaDirectorio), ruta);
        g_free(ruta);
    }

    g_object_unref(carpeta);
}

static void alPresionarExaminar(GtkButton *boton, gpointer datos)
{
    (void)boton;

    Aplicacion *app = datos;

    GtkFileDialog *dialogo = gtk_file_dialog_new();

    gtk_file_dialog_set_title(dialogo, "Escoja el directorio con los archivos .txt");
    gtk_file_dialog_select_folder(dialogo, app->ventana, NULL, alEscogerCarpeta, app);

    g_object_unref(dialogo);
}

/* ---------------------------------------------------------------
   Construccion de la ventana
   --------------------------------------------------------------- */

static GtkWidget *etiquetaTitulo(const char *texto)
{
    GtkWidget *etiqueta = gtk_label_new(texto);

    gtk_widget_add_css_class(etiqueta, "heading");
    gtk_label_set_xalign(GTK_LABEL(etiqueta), 0.0);
    gtk_widget_set_margin_start(etiqueta, 6);
    gtk_widget_set_margin_end(etiqueta, 6);

    return etiqueta;
}

static GtkWidget *etiquetaCelda(const char *texto)
{
    GtkWidget *etiqueta = gtk_label_new(texto);

    gtk_label_set_xalign(GTK_LABEL(etiqueta), 0.0);
    gtk_widget_set_margin_start(etiqueta, 6);
    gtk_widget_set_margin_end(etiqueta, 6);

    return etiqueta;
}

static void construirVentana(GtkApplication *gtkApp, gpointer datos)
{
    Aplicacion *app = datos;

    GtkWidget *ventana = gtk_application_window_new(gtkApp);

    app->ventana = GTK_WINDOW(ventana);

    gtk_window_set_title(GTK_WINDOW(ventana), "Compresor Huffman — Proyecto 1");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 1000, 420);

    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    gtk_widget_set_margin_top(caja, 16);
    gtk_widget_set_margin_bottom(caja, 16);
    gtk_widget_set_margin_start(caja, 16);
    gtk_widget_set_margin_end(caja, 16);

    /* --- Fila del directorio --- */
    GtkWidget *filaDirectorio = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    GtkWidget *etiqueta = gtk_label_new("Directorio:");

    app->entradaDirectorio = gtk_entry_new();
    gtk_widget_set_hexpand(app->entradaDirectorio, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->entradaDirectorio),
                                   "Ruta del directorio con los archivos .txt");

    GtkWidget *botonExaminar = gtk_button_new_with_label("Examinar...");

    g_signal_connect(botonExaminar, "clicked", G_CALLBACK(alPresionarExaminar), app);

    gtk_box_append(GTK_BOX(filaDirectorio), etiqueta);
    gtk_box_append(GTK_BOX(filaDirectorio), app->entradaDirectorio);
    gtk_box_append(GTK_BOX(filaDirectorio), botonExaminar);

    gtk_box_append(GTK_BOX(caja), filaDirectorio);

    /* --- Boton de ejecucion --- */
    app->botonEjecutar = gtk_button_new_with_label(
        "Comprimir y descomprimir con las 3 versiones");

    g_signal_connect(app->botonEjecutar, "clicked", G_CALLBACK(alPresionarEjecutar), app);

    gtk_box_append(GTK_BOX(caja), app->botonEjecutar);

    /* --- Tabla comparativa --- */
    GtkWidget *rejilla = gtk_grid_new();

    gtk_grid_set_row_spacing(GTK_GRID(rejilla), 8);
    gtk_grid_set_column_spacing(GTK_GRID(rejilla), 12);

    for (int columna = 0; columna <= CANTIDAD_COLUMNAS; columna++) {
        gtk_grid_attach(GTK_GRID(rejilla),
                        etiquetaTitulo(TITULOS_COLUMNA[columna]), columna, 0, 1, 1);
    }

    for (int fila = 0; fila < CANTIDAD_VERSIONES; fila++) {
        gtk_grid_attach(GTK_GRID(rejilla),
                        etiquetaCelda(NOMBRES_VERSION[fila]), 0, fila + 1, 1, 1);

        for (int columna = 0; columna < CANTIDAD_COLUMNAS; columna++) {
            app->celdas[fila][columna] = etiquetaCelda("—");
            gtk_grid_attach(GTK_GRID(rejilla),
                            app->celdas[fila][columna], columna + 1, fila + 1, 1, 1);
        }
    }

    GtkWidget *marco = gtk_frame_new("Estadisticas comparativas");

    gtk_widget_set_margin_top(rejilla, 12);
    gtk_widget_set_margin_bottom(rejilla, 12);
    gtk_widget_set_margin_start(rejilla, 12);
    gtk_widget_set_margin_end(rejilla, 12);
    gtk_frame_set_child(GTK_FRAME(marco), rejilla);

    gtk_box_append(GTK_BOX(caja), marco);

    /* --- Estado --- */
    app->etiquetaEstado = gtk_label_new("Escoja un directorio y presione el boton.");
    gtk_label_set_xalign(GTK_LABEL(app->etiquetaEstado), 0.0);
    gtk_label_set_wrap(GTK_LABEL(app->etiquetaEstado), TRUE);

    gtk_box_append(GTK_BOX(caja), app->etiquetaEstado);

    gtk_window_set_child(GTK_WINDOW(ventana), caja);
    gtk_window_present(GTK_WINDOW(ventana));
}

int main(int argc, char *argv[])
{
    Aplicacion app;

    memset(&app, 0, sizeof(app));

    GtkApplication *gtkApp = gtk_application_new("cr.ac.itcr.compresorhuffman",
                                                 G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(gtkApp, "activate", G_CALLBACK(construirVentana), &app);

    int estado = g_application_run(G_APPLICATION(gtkApp), argc, argv);

    g_object_unref(gtkApp);

    return estado;
}
