/* Interfaz grafica del Proyecto 1 — Compresor Huffman
 *
 * Usa GTK 4, que es el toolkit nativo de GNOME en Debian 13.
 *
 * Las corridas se ejecutan en un hilo aparte para que la ventana no se
 * congele. GTK exige que los widgets se toquen SOLO desde el hilo
 * principal, asi que el hilo de trabajo nunca modifica la interfaz:
 * cuando termina cada corrida, le pasa el resultado al hilo principal
 * con g_idle_add() y es este quien actualiza la tabla.
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

static const char *CARPETAS_VERSION[CANTIDAD_VERSIONES] = {
    "serial", "fork", "hilos"
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

/* Columnas que llena cada tipo de corrida */
static const int COLUMNAS_COMPRESION[]    = { 1, 3, 5, 6, 7 };
static const int COLUMNAS_DESCOMPRESION[] = { 0, 2, 4 };

typedef int (*FuncionCorrida)(const char *, const char *, Estadisticas *);

static const FuncionCorrida COMPRESORES[CANTIDAD_VERSIONES] = {
    comprimirSerial, comprimirParalelo, comprimirConcurrente
};

static const FuncionCorrida DESCOMPRESORES[CANTIDAD_VERSIONES] = {
    descomprimirSerial, descomprimirParalelo, descomprimirConcurrente
};

typedef enum {
    OPERACION_COMPRIMIR,
    OPERACION_DESCOMPRIMIR
} Operacion;

typedef struct {
    GtkWindow *ventana;
    GtkWidget *entradaDirectorio;
    GtkWidget *entradaPaquete;
    GtkWidget *botonComprimir;
    GtkWidget *botonDescomprimir;
    GtkWidget *etiquetaEstado;
    GtkWidget *celdas[CANTIDAD_VERSIONES][CANTIDAD_COLUMNAS];

    /* Tiempos de la version serial, referencia para las aceleraciones */
    double segundosCompresionSerial;
    double segundosDescompresionSerial;

    /* Estado de la corrida en curso. Solo se leen y escriben desde el
       hilo principal, asi que no necesitan candado. */
    gboolean ejecutando;
    gboolean huboMensaje;
} Aplicacion;

/* Lo que recibe el hilo de trabajo. La ruta es una COPIA del texto del
   campo: el usuario podria editar el campo mientras las corridas avanzan. */
typedef struct {
    Aplicacion *app;
    Operacion operacion;
    char *ruta;
} Trabajo;

/* Resultado de una fila, que el hilo de trabajo le pasa al principal. */
typedef struct {
    Aplicacion *app;
    Operacion operacion;
    int fila;
    Estadisticas est;
    char rutaPaquete[4096];
} ResultadoFila;

/* ---------------------------------------------------------------
   Utilidades de la tabla (solo hilo principal)
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

static void ponerEstado(Aplicacion *app, const char *texto)
{
    gtk_label_set_text(GTK_LABEL(app->etiquetaEstado), texto);
}

static void bloquearBotones(Aplicacion *app, gboolean bloquear)
{
    gtk_widget_set_sensitive(app->botonComprimir, !bloquear);
    gtk_widget_set_sensitive(app->botonDescomprimir, !bloquear);
}

/* ---------------------------------------------------------------
   Rutas de resultados (no tocan GTK: seguras en cualquier hilo)
   --------------------------------------------------------------- */

static int prepararBaseVersion(const char *version, char *base, size_t tamano)
{
    if (!asegurarDirectorio("resultados")) {
        return 0;
    }

    if ((int)tamano <= snprintf(base, tamano, "resultados/%s", version)) {
        return 0;
    }

    return asegurarDirectorio(base);
}

/* resultados/<version>/paquete.huff, donde escribe el compresor */
static int rutaPaqueteVersion(const char *version, char *rutaPaquete, size_t tamano)
{
    char base[2048];

    if (!prepararBaseVersion(version, base, sizeof(base))) {
        return 0;
    }

    return snprintf(rutaPaquete, tamano, "%s/paquete.huff", base) < (int)tamano;
}

/* resultados/<version>/descomprimidos, donde escribe el descompresor */
static int prepararSalidaVersion(const char *version, char *dirDescomprimido,
                                 size_t tamano)
{
    char base[2048];

    if (!prepararBaseVersion(version, base, sizeof(base))) {
        return 0;
    }

    if ((int)tamano <= snprintf(dirDescomprimido, tamano, "%s/descomprimidos", base)) {
        return 0;
    }

    return asegurarDirectorio(dirDescomprimido);
}

/* ---------------------------------------------------------------
   Callbacks que el hilo de trabajo programa en el hilo principal
   --------------------------------------------------------------- */

/* Pinta en la tabla el resultado de una version. Corre en el hilo
   principal. GLib ejecuta las fuentes g_idle_add en el orden en que se
   agregan, asi que la fila serial siempre llega antes que las demas y
   el tiempo de referencia ya esta disponible para las aceleraciones. */
static gboolean mostrarFila(gpointer datos)
{
    ResultadoFila *resultado = datos;
    Aplicacion *app = resultado->app;
    const Estadisticas *est = &resultado->est;
    int i = resultado->fila;
    char texto[64];

    if (resultado->operacion == OPERACION_COMPRIMIR) {
        if (i == 0) {
            app->segundosCompresionSerial = est->ok ? est->segundos : 0.0;
        }

        if (est->ok) {
            snprintf(texto, sizeof(texto), "%.6f", est->segundos);
            ponerCelda(app, i, 1, texto);

            snprintf(texto, sizeof(texto), "%zu", est->bytesOriginales);
            ponerCelda(app, i, 5, texto);

            snprintf(texto, sizeof(texto), "%zu", est->bytesComprimidos);
            ponerCelda(app, i, 6, texto);

            snprintf(texto, sizeof(texto), "%.3f", estadisticasRazon(est));
            ponerCelda(app, i, 7, texto);
        }

        if (est->ok && 0.0 < app->segundosCompresionSerial) {
            snprintf(texto, sizeof(texto), "%.1f",
                     estadisticasAceleracion(app->segundosCompresionSerial,
                                             est->segundos));
            ponerCelda(app, i, 3, texto);
        }

        /* Dejar el paquete serial recien creado como sugerencia */
        if (i == 0 && est->ok) {
            gtk_editable_set_text(GTK_EDITABLE(app->entradaPaquete),
                                  resultado->rutaPaquete);
        }
    }
    else {
        if (i == 0) {
            app->segundosDescompresionSerial = est->ok ? est->segundos : 0.0;
        }

        if (est->ok) {
            snprintf(texto, sizeof(texto), "%.1f", estadisticasSalud(est));
            ponerCelda(app, i, 0, texto);

            snprintf(texto, sizeof(texto), "%.6f", est->segundos);
            ponerCelda(app, i, 2, texto);
        }

        if (est->ok && 0.0 < app->segundosDescompresionSerial) {
            snprintf(texto, sizeof(texto), "%.1f",
                     estadisticasAceleracion(app->segundosDescompresionSerial,
                                             est->segundos));
            ponerCelda(app, i, 4, texto);
        }
    }

    if (est->mensaje[0] != '\0') {
        ponerEstado(app, est->mensaje);
        app->huboMensaje = TRUE;
    }
    else if (!app->huboMensaje && i + 1 < CANTIDAD_VERSIONES) {
        /* Avance: avisar cual version sigue */
        char avance[128];

        snprintf(avance, sizeof(avance), "%s: version %s lista, ejecutando %s...",
                 resultado->operacion == OPERACION_COMPRIMIR ? "Comprimiendo"
                                                             : "Descomprimiendo",
                 NOMBRES_VERSION[i], NOMBRES_VERSION[i + 1]);
        ponerEstado(app, avance);
    }

    g_free(resultado);

    return G_SOURCE_REMOVE;
}

/* Se ejecuta al final, en el hilo principal: reactiva la interfaz. */
static gboolean terminarCorridas(gpointer datos)
{
    Trabajo *trabajo = datos;
    Aplicacion *app = trabajo->app;

    app->ejecutando = FALSE;
    bloquearBotones(app, FALSE);

    if (!app->huboMensaje) {
        if (trabajo->operacion == OPERACION_COMPRIMIR) {
            ponerEstado(app, "Compresion lista. El paquete quedo en "
                             "resultados/<version>/paquete.huff.");
        }
        else {
            ponerEstado(app, "Descompresion lista. Los archivos quedaron en "
                             "resultados/<version>/descomprimidos.");
        }
    }

    g_free(trabajo->ruta);
    g_free(trabajo);

    return G_SOURCE_REMOVE;
}

/* ---------------------------------------------------------------
   Hilo de trabajo
   ---------------------------------------------------------------
   IMPORTANTE: esta funcion NO corre en el hilo principal, por lo que
   no puede llamar a ninguna funcion de GTK. Todo lo que tiene que
   mostrar lo manda con g_idle_add(). */

static gpointer hiloCorridas(gpointer datos)
{
    Trabajo *trabajo = datos;

    for (int i = 0; i < CANTIDAD_VERSIONES; i++) {
        ResultadoFila *resultado = g_new0(ResultadoFila, 1);

        resultado->app = trabajo->app;
        resultado->operacion = trabajo->operacion;
        resultado->fila = i;

        if (trabajo->operacion == OPERACION_COMPRIMIR) {
            if (rutaPaqueteVersion(CARPETAS_VERSION[i], resultado->rutaPaquete,
                                   sizeof(resultado->rutaPaquete))) {
                COMPRESORES[i](trabajo->ruta, resultado->rutaPaquete, &resultado->est);
            }
            else {
                estadisticasIniciar(&resultado->est);
                snprintf(resultado->est.mensaje, sizeof(resultado->est.mensaje),
                         "No se pudieron crear las carpetas de resultados.");
            }
        }
        else {
            char dirDescomprimido[4096];

            if (prepararSalidaVersion(CARPETAS_VERSION[i], dirDescomprimido,
                                      sizeof(dirDescomprimido))) {
                DESCOMPRESORES[i](trabajo->ruta, dirDescomprimido, &resultado->est);
            }
            else {
                estadisticasIniciar(&resultado->est);
                snprintf(resultado->est.mensaje, sizeof(resultado->est.mensaje),
                         "No se pudieron crear las carpetas de resultados.");
            }
        }

        g_idle_add(mostrarFila, resultado);
    }

    g_idle_add(terminarCorridas, trabajo);

    return NULL;
}

/* Arranca las tres corridas en segundo plano. Corre en el hilo principal. */
static void lanzarCorridas(Aplicacion *app, Operacion operacion, const char *ruta)
{
    app->ejecutando = TRUE;
    app->huboMensaje = FALSE;

    if (operacion == OPERACION_COMPRIMIR) {
        app->segundosCompresionSerial = 0.0;
        limpiarColumnas(app, COLUMNAS_COMPRESION,
                        (int)(sizeof(COLUMNAS_COMPRESION) / sizeof(int)));
        ponerEstado(app, "Comprimiendo: ejecutando version Serial...");
    }
    else {
        app->segundosDescompresionSerial = 0.0;
        limpiarColumnas(app, COLUMNAS_DESCOMPRESION,
                        (int)(sizeof(COLUMNAS_DESCOMPRESION) / sizeof(int)));
        ponerEstado(app, "Descomprimiendo: ejecutando version Serial...");
    }

    bloquearBotones(app, TRUE);

    Trabajo *trabajo = g_new0(Trabajo, 1);

    trabajo->app = app;
    trabajo->operacion = operacion;
    trabajo->ruta = g_strdup(ruta);

    /* El hilo se desprende: nadie espera por el con join. Avisa que
       termino a traves de terminarCorridas(). */
    GThread *hilo = g_thread_new("corridas", hiloCorridas, trabajo);

    g_thread_unref(hilo);
}

/* ---------------------------------------------------------------
   Botones
   --------------------------------------------------------------- */

static void alPresionarComprimir(GtkButton *boton, gpointer datos)
{
    (void)boton;

    Aplicacion *app = datos;

    const char *directorio = gtk_editable_get_text(GTK_EDITABLE(app->entradaDirectorio));

    if (directorio == NULL || directorio[0] == '\0') {
        ponerEstado(app, "Escoja primero un directorio con archivos .txt.");

        return;
    }

    lanzarCorridas(app, OPERACION_COMPRIMIR, directorio);
}

static void alPresionarDescomprimir(GtkButton *boton, gpointer datos)
{
    (void)boton;

    Aplicacion *app = datos;

    const char *rutaPaquete = gtk_editable_get_text(GTK_EDITABLE(app->entradaPaquete));

    if (rutaPaquete == NULL || rutaPaquete[0] == '\0') {
        ponerEstado(app, "Escoja primero un paquete .huff para descomprimir.");

        return;
    }

    lanzarCorridas(app, OPERACION_DESCOMPRIMIR, rutaPaquete);
}

/* No permitir cerrar la ventana a mitad de una corrida: el hilo de
   trabajo todavia le enviaria resultados a widgets ya destruidos. */
static gboolean alPedirCierre(GtkWindow *ventana, gpointer datos)
{
    (void)ventana;

    Aplicacion *app = datos;

    if (app->ejecutando) {
        ponerEstado(app, "Espere a que terminen las corridas antes de cerrar.");

        return TRUE;
    }

    return FALSE;
}

/* ---------------------------------------------------------------
   Selectores de archivo y carpeta
   --------------------------------------------------------------- */

static void alEscogerCarpeta(GObject *fuente, GAsyncResult *resultado, gpointer datos)
{
    Aplicacion *app = datos;
    GError *error = NULL;

    GFile *carpeta = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(fuente),
                                                          resultado, &error);

    if (carpeta == NULL) {
        /* El usuario cancelo */
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

static void alEscogerPaquete(GObject *fuente, GAsyncResult *resultado, gpointer datos)
{
    Aplicacion *app = datos;
    GError *error = NULL;

    GFile *archivo = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(fuente),
                                                 resultado, &error);

    if (archivo == NULL) {
        if (error != NULL) {
            g_error_free(error);
        }

        return;
    }

    char *ruta = g_file_get_path(archivo);

    if (ruta != NULL) {
        gtk_editable_set_text(GTK_EDITABLE(app->entradaPaquete), ruta);
        g_free(ruta);
    }

    g_object_unref(archivo);
}

static void alPresionarExaminarPaquete(GtkButton *boton, gpointer datos)
{
    (void)boton;

    Aplicacion *app = datos;

    GtkFileDialog *dialogo = gtk_file_dialog_new();

    gtk_file_dialog_set_title(dialogo, "Escoja el paquete comprimido (.huff)");
    gtk_file_dialog_open(dialogo, app->ventana, NULL, alEscogerPaquete, app);

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

static GtkWidget *filaSelector(const char *titulo, const char *sugerencia,
                               GtkWidget **entrada, GCallback alExaminar,
                               Aplicacion *app)
{
    GtkWidget *fila = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *etiqueta = gtk_label_new(titulo);

    *entrada = gtk_entry_new();
    gtk_widget_set_hexpand(*entrada, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(*entrada), sugerencia);

    GtkWidget *boton = gtk_button_new_with_label("Examinar...");

    g_signal_connect(boton, "clicked", alExaminar, app);

    gtk_box_append(GTK_BOX(fila), etiqueta);
    gtk_box_append(GTK_BOX(fila), *entrada);
    gtk_box_append(GTK_BOX(fila), boton);

    return fila;
}

static void construirVentana(GtkApplication *gtkApp, gpointer datos)
{
    Aplicacion *app = datos;

    GtkWidget *ventana = gtk_application_window_new(gtkApp);

    app->ventana = GTK_WINDOW(ventana);

    gtk_window_set_title(GTK_WINDOW(ventana), "Compresor Huffman — Proyecto 1");
    gtk_window_set_default_size(GTK_WINDOW(ventana), 900, 400);

    g_signal_connect(ventana, "close-request", G_CALLBACK(alPedirCierre), app);

    GtkWidget *caja = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    gtk_widget_set_margin_top(caja, 16);
    gtk_widget_set_margin_bottom(caja, 16);
    gtk_widget_set_margin_start(caja, 16);
    gtk_widget_set_margin_end(caja, 16);

    /* --- Selectores --- */
    gtk_box_append(GTK_BOX(caja),
                   filaSelector("Directorio:", "Ruta del directorio con los archivos .txt",
                                &app->entradaDirectorio,
                                G_CALLBACK(alPresionarExaminar), app));

    gtk_box_append(GTK_BOX(caja),
                   filaSelector("Paquete:", "Ruta del archivo .huff a descomprimir",
                                &app->entradaPaquete,
                                G_CALLBACK(alPresionarExaminarPaquete), app));

    /* --- Botones --- */
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
    app->etiquetaEstado = gtk_label_new("Escoja un directorio y presione Comprimir.");
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