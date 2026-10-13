#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "../include/os_detect.h"
#include "../include/installer.h"
#include "../include/ui.h"

#define MAX_APPS 100

typedef struct {
    GtkWidget *window;
    GtkWidget *install_btn;
    GtkWidget *checkboxes[MAX_APPS];
    Package *apps;
    int app_count;
    int os_type;
} AppContext;

typedef struct {
    AppContext *context;
    char *packages;
    int result;
} InstallJob;

static void install_job_free(gpointer data) {
    InstallJob *job = data;
    g_free(job->packages);
    g_free(job);
}

/* Runs in a worker thread so the window stays responsive during the install. */
static void install_thread(GTask *task, gpointer source, gpointer task_data, GCancellable *cancellable) {
    (void) source; (void) cancellable;
    InstallJob *job = task_data;
    job->result = install_packages(job->context->os_type, job->packages);
    g_task_return_boolean(task, TRUE);
}

/* Back in the GTK main thread. */
static void install_done(GObject *source, GAsyncResult *res, gpointer user_data) {
    (void) source; (void) user_data;
    GTask *task = G_TASK(res);
    InstallJob *job = g_task_get_task_data(task);
    AppContext *context = job->context;

    gtk_widget_set_sensitive(context->install_btn, TRUE);

    GtkMessageType type = (job->result == 0) ? GTK_MESSAGE_INFO : GTK_MESSAGE_ERROR;
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(context->window),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               type, GTK_BUTTONS_OK,
                                               "%s",
                                               job->result == 0
                                                   ? "Installation finished successfully."
                                                   : "Installation failed. Check that every selected package exists in your repositories.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_install_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    AppContext *context = (AppContext *) user_data;
    GString *selected = g_string_new(NULL);

    for (int i = 0; i < context->app_count; i++) {
        if (context->checkboxes[i] != NULL &&
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(context->checkboxes[i]))) {
            if (selected->len > 0) g_string_append_c(selected, ' ');
            g_string_append(selected, context->apps[i].package_name);
        }
    }

    if (selected->len == 0) {
        g_string_free(selected, TRUE);
        return;
    }

    InstallJob *job = g_new0(InstallJob, 1);
    job->context = context;
    job->packages = g_string_free(selected, FALSE);

    gtk_widget_set_sensitive(context->install_btn, FALSE);

    GTask *task = g_task_new(NULL, NULL, install_done, NULL);
    g_task_set_task_data(task, job, install_job_free);
    g_task_run_in_thread(task, install_thread);
    g_object_unref(task);
}

void run_ui(int os_type) {
    gtk_init(NULL, NULL);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "NiniX - Software Installer");
    gtk_window_set_default_size(GTK_WINDOW(window), 550, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *header_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header_label), "<span size='x-large' weight='bold'>NiniX Installer</span>\n<span color='gray'>Select apps to install. Powered by packages.json</span>");
    gtk_label_set_xalign(GTK_LABEL(header_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), header_label, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_add(GTK_CONTAINER(scrolled), content_box);

    static Package apps[MAX_APPS];
    int loaded_count = load_packages_from_json(os_type, apps, MAX_APPS);

    AppContext *context = g_new0(AppContext, 1);
    context->window = window;
    context->apps = apps;
    context->app_count = loaded_count;
    context->os_type = os_type;

    if (loaded_count == 0) {
        GtkWidget *empty = gtk_label_new("No packages found.\nCheck that data/packages.json exists and is valid.");
        gtk_box_pack_start(GTK_BOX(content_box), empty, FALSE, FALSE, 0);
    }

    char unique_categories[MAX_APPS][PKG_CATEGORY_LEN];
    int unique_cat_count = 0;

    for (int i = 0; i < loaded_count; i++) {
        int exists = 0;
        for (int c = 0; c < unique_cat_count; c++) {
            if (strcasecmp(unique_categories[c], apps[i].category) == 0) {
                exists = 1;
                break;
            }
        }
        if (!exists && unique_cat_count < MAX_APPS) {
            snprintf(unique_categories[unique_cat_count], PKG_CATEGORY_LEN, "%s", apps[i].category);
            unique_cat_count++;
        }
    }

    for (int c = 0; c < unique_cat_count; c++) {
        GtkWidget *frame = gtk_frame_new(unique_categories[c]);
        gtk_box_pack_start(GTK_BOX(content_box), frame, FALSE, FALSE, 0);

        GtkWidget *cat_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(cat_vbox), 10);
        gtk_container_add(GTK_CONTAINER(frame), cat_vbox);

        for (int i = 0; i < loaded_count; i++) {
            if (strcasecmp(apps[i].category, unique_categories[c]) == 0) {
                char label_text[PKG_NAME_LEN + PKG_PACKAGE_LEN + 8];
                snprintf(label_text, sizeof(label_text), "%s  (%s)", apps[i].name, apps[i].package_name);

                context->checkboxes[i] = gtk_check_button_new_with_label(label_text);
                gtk_box_pack_start(GTK_BOX(cat_vbox), context->checkboxes[i], FALSE, FALSE, 0);
            }
        }
    }

    GtkWidget *install_btn = gtk_button_new_with_label("Install Now");
    context->install_btn = install_btn;
    GtkStyleContext *btn_style = gtk_widget_get_style_context(install_btn);
    gtk_style_context_add_class(btn_style, "suggested-action");

    g_signal_connect(install_btn, "clicked", G_CALLBACK(on_install_clicked), context);
    gtk_box_pack_start(GTK_BOX(vbox), install_btn, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    g_free(context);
}
