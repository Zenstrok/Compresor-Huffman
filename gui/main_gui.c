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
    "T. compr. (s)",
    "T. descompr. (s)",
    "Acel. compr. (%)",
    "Acel. descompr. (%)",
    "Original (B)",
    "Comprimido (B)",
    "Razon"
};

typedef struct {
    GtkWindow *ventana;
    GtkWidget *entradaDirectorio;
    GtkWidget *botonComprimir;
    GtkWidget *botonDescomprimir;
    GtkWidget *etiquetaEstado;
    GtkWidget *celdas[CANTIDAD_VERSIONES][CANTIDAD_COLUMNAS];

    /* Tiempos de la version serial, que sirven de referencia para
       calcular las aceleraciones. Se guardan aqui porque ahora la
       compresion y la descompresion se ejecutan por separado. */
    double segundosCompresionSerial;
    double segundosDescompresionSerial;
} Aplicacion;

/* ---------------------------------------------------------------
   Utilidades
   --------------------------------------------------------------- */

static void ponerCelda(Aplicacion *app, int fila, int columna, const char *texto)
{
    gtk_label_set_text(GTK_LABEL(app->celdas[fila][columna]), texto);
}

static void limpiarColumnas(Aplicacion *app, const int *columnas, int cantidad)
{
    for (int fila = 0; fila < CANTIDAD_VERSIONES; fila++) {
        for (int i = 0; i < cantidad; i++) {
            ponerCelda(app, fila, columnas[i], "—");
        }
    }
}

/* Columnas que llena cada corrida */
static const int COLUMNAS_COMPRESION[]    = { 1, 3, 5, 6, 7 };
static const int COLUMNAS_DESCOMPRESION[] = { 0, 2, 4 };

typedef int (*FuncionCorrida)(const char *, const char *, Estadisticas *);

/* Arma resultados/<version>/paquete.huff y resultados/<version>/descomprimidos */
static int prepararRutasVersion(const char *version, char *rutaPaquete,
                                char *dirDescomprimido, size_t tamano)
{
    char base[2048];

    if (!asegurarDirectorio("resultados")) {
        return 0;
    }

    if ((int)sizeof(base) <= snprintf(base, sizeof(base), "resultados/%s", version)) {
        return 0;
    }

    if (!asegurarDirectorio(base)) {
        return 0;
    }

    if ((int)tamano <= snprintf(rutaPaquete, tamano, "%s/paquete.huff", base)) {
        return 0;
    }

    if ((int)tamano <= snprintf(dirDescomprimido, tamano, "%s/descomprimidos", base)) {
        return 0;
    }

    return asegurarDirectorio(dirDescomprimido);
}

static void ejecutarCompresion(Aplicacion *app, const char *directorio)
{
    FuncionCorrida compresores[CANTIDAD_VERSIONES] = {
        comprimirSerial, comprimirParalelo, comprimirConcurrente
    };

    const char *carpetas[CANTIDAD_VERSIONES] = { "serial", "fork", "hilos" };

    app->segundosCompresionSerial = 0.0;

    for (int i = 0; i < CANTIDAD_VERSIONES; i++) {
        char rutaPaquete[4096];
        char dirDescomprimido[4096];

        if (!prepararRutasVersion(carpetas[i], rutaPaquete,
                                  dirDescomprimido, sizeof(rutaPaquete))) {
            gtk_label_set_text(GTK_LABEL(app->etiquetaEstado),
                               "No se pudieron crear las carpetas de resultados.");

            continue;
        }

        Estadisticas est;

        compresores[i](directorio, rutaPaquete, &est);

        if (i == 0 && est.ok) {
            app->segundosCompresionSerial = est.segundos;
        }

        char texto[64];

        if (est.ok) {
            snprintf(texto, sizeof(texto), "%.6f", est.segundos);
            ponerCelda(app, i, 1, texto);

            snprintf(texto, sizeof(texto), "%zu", est.bytesOriginales);
            ponerCelda(app, i, 5, texto);

            snprintf(texto, sizeof(texto), "%zu", est.bytesComprimidos);
            ponerCelda(app, i, 6, texto);

            snprintf(texto, sizeof(texto), "%.3f", estadisticasRazon(&est));
            ponerCelda(app, i, 7, texto);
        }

        if (est.ok && 0.0 < app->segundosCompresionSerial) {
            snprintf(texto, sizeof(texto), "%.1f",
                     estadisticasAceleracion(app->segundosCompresionSerial,
                                             est.segundos));
            ponerCelda(app, i, 3, texto);
        }

        if (est.mensaje[0] != '\0') {
            gtk_label_set_text(GTK_LABEL(app->etiquetaEstado), est.mensaje);
        }
    }
}

static void ejecutarDescompresion(Aplicacion *app)
{
    FuncionCorrida descompresores[CANTIDAD_VERSIONES] = {
        descomprimirSerial, descomprimirParalelo, descomprimirConcurrente
    };

    const char *carpetas[CANTIDAD_VERSIONES] = { "serial", "fork", "hilos" };

    app->segundosDescompresionSerial = 0.0;

    for (int i = 0; i < CANTIDAD_VERSIONES; i++) {
        char rutaPaquete[4096];
        char dirDescomprimido[4096];

        if (!prepararRutasVersion(carpetas[i], rutaPaquete,
                                  dirDescomprimido, sizeof(rutaPaquete))) {
            gtk_label_set_text(GTK_LABEL(app->etiquetaEstado),
                               "No se pudieron crear las carpetas de resultados.");

            continue;
        }

        Estadisticas est;

        descompresores[i](rutaPaquete, dirDescomprimido, &est);

        if (i == 0 && est.ok) {
            app->segundosDescompresionSerial = est.segundos;
        }

        char texto[64];

        if (est.ok) {
            snprintf(texto, sizeof(texto), "%.1f", estadisticasSalud(&est));
            ponerCelda(app, i, 0, texto);

            snprintf(texto, sizeof(texto), "%.6f", est.segundos);
            ponerCelda(app, i, 2, texto);
        }

        if (est.ok && 0.0 < app->segundosDescompresionSerial) {
            snprintf(texto, sizeof(texto), "%.1f",
                     estadisticasAceleracion(app->segundosDescompresionSerial,
                                             est.segundos));
            ponerCelda(app, i, 4, texto);
        }

        if (est.mensaje[0] != '\0') {
            gtk_label_set_text(GTK_LABEL(app->etiquetaEstado), est.mensaje);
        }
    }
}

static void alPresionarComprimir(GtkButton *boton, gpointer datos)
{
    (void)boton;

    Aplicacion *app = datos;

    const char *directorio = gtk_editable_get_text(GTK_EDITABLE(app->entradaDirectorio));

    if (directorio == NULL || directorio[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(app->etiquetaEstado),
                           "Escoja primero un directorio con archivos .txt.");

        return;
    }

    limpiarColumnas(app, COLUMNAS_COMPRESION,
                    (int)(sizeof(COLUMNAS_COMPRESION) / sizeof(int)));

    gtk_label_set_text(GTK_LABEL(app->etiquetaEstado), "Comprimiendo...");
    gtk_widget_set_sensitive(app->botonComprimir, FALSE);
    gtk_widget_set_sensitive(app->botonDescomprimir, FALSE);

    ejecutarCompresion(app, directorio);

    gtk_widget_set_sensitive(app->botonComprimir, TRUE);
    gtk_widget_set_sensitive(app->botonDescomprimir, TRUE);

    if (strcmp(gtk_label_get_text(GTK_LABEL(app->etiquetaEstado)), "Comprimiendo...") == 0) {
        gtk_label_set_text(GTK_LABEL(app->etiquetaEstado),
                           "Compresion lista. El paquete quedo en resultados/<version>/paquete.huff.");
    }
}

static void alPresionarDescomprimir(GtkButton *boton, gpointer datos)
{
    (void)boton;

    Aplicacion *app = datos;

    limpiarColumnas(app, COLUMNAS_DESCOMPRESION,
                    (int)(sizeof(COLUMNAS_DESCOMPRESION) / sizeof(int)));

    gtk_label_set_text(GTK_LABEL(app->etiquetaEstado), "Descomprimiendo...");
    gtk_widget_set_sensitive(app->botonComprimir, FALSE);
    gtk_widget_set_sensitive(app->botonDescomprimir, FALSE);

    ejecutarDescompresion(app);

    gtk_widget_set_sensitive(app->botonComprimir, TRUE);
    gtk_widget_set_sensitive(app->botonDescomprimir, TRUE);

    if (strcmp(gtk_label_get_text(GTK_LABEL(app->etiquetaEstado)), "Descomprimiendo...") == 0) {
        gtk_label_set_text(GTK_LABEL(app->etiquetaEstado),
                           "Descompresion lista. Los .txt quedaron en resultados/<version>/descomprimidos.");
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
        gtk_window_set_default_size(GTK_WINDOW(ventana), 900, 400);

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

        /* --- Botones separados --- */
    GtkWidget *filaBotones = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    app->botonComprimir = gtk_button_new_with_label("Comprimir con las 3 versiones");
    app->botonDescomprimir = gtk_button_new_with_label("Descomprimir con las 3 versiones");

    gtk_widget_set_hexpand(app->botonComprimir, TRUE);
    gtk_widget_set_hexpand(app->botonDescomprimir, TRUE);

    g_signal_connect(app->botonComprimir, "clicked",
                     G_CALLBACK(alPresionarComprimir), app);
    g_signal_connect(app->botonDescomprimir, "clicked",
                     G_CALLBACK(alPresionarDescomprimir), app);

    gtk_box_append(GTK_BOX(filaBotones), app->botonComprimir);
    gtk_box_append(GTK_BOX(filaBotones), app->botonDescomprimir);

    gtk_box_append(GTK_BOX(caja), filaBotones);

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

    gtk_widget_set_margin_top(rejilla, 12);
    gtk_widget_set_margin_bottom(rejilla, 12);
    gtk_widget_set_margin_start(rejilla, 12);
    gtk_widget_set_margin_end(rejilla, 12);

    /* La tabla es mas ancha que muchas pantallas, asi que va dentro de
       un contenedor con barras de desplazamiento */
    GtkWidget *desplazable = gtk_scrolled_window_new();

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(desplazable),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(desplazable), rejilla);
    gtk_widget_set_hexpand(desplazable, TRUE);
    gtk_widget_set_vexpand(desplazable, TRUE);

    GtkWidget *marco = gtk_frame_new("Estadisticas comparativas");

    gtk_frame_set_child(GTK_FRAME(marco), desplazable);

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
