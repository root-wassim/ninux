#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/os_detect.h"
#include "../include/installer.h"
#include "../include/ui.h"

typedef struct {
    GtkWidget *checkboxes[100];
    Package *apps;
    int app_count;
    int os_type;
} AppContext;



static void on_install_clicked(GtkButton *button, gpointer user_data) {
    AppContext *context = (AppContext *)user_data;
    char selected_packages[2048] = "";



    for (int i = 0; i < context->app_count; i++) {
        if (context->checkboxes[i] != NULL && 
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(context->checkboxes[i]))) {
            strcat(selected_packages, context->apps[i].package_name);
            strcat(selected_packages, " ");
        }
    }

    if (strlen(selected_packages) == 0) {
        return;
    }

    install_packages(context->os_type, selected_packages);
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

    static Package apps[100];
    int loaded_count = load_packages_from_json(os_type, apps, 100);


    

    AppContext *context = g_malloc(sizeof(AppContext));
    context->apps = apps;
    context->app_count = loaded_count;
    context->os_type = os_type;

    for (int i = 0; i < 100; i++) context->checkboxes[i] = NULL;

    char unique_categories[50][50];
    int unique_cat_count = 0;

    for (int i = 0; i < loaded_count; i++) {
        int exists = 0;
        for (int c = 0; c < unique_cat_count; c++) {
            if (strcasecmp(unique_categories[c], apps[i].category) == 0) {
                exists = 1;
                break;
            }
        }
        if (!exists && unique_cat_count < 50) {
            strncpy(unique_categories[unique_cat_count], apps[i].category, 50);
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
                char label_text[256];
                snprintf(label_text, sizeof(label_text), "%s  (%s)", apps[i].name, apps[i].package_name);
                
                context->checkboxes[i] = gtk_check_button_new_with_label(label_text);
                gtk_box_pack_start(GTK_BOX(cat_vbox), context->checkboxes[i], FALSE, FALSE, 0);
            }
        }
    }



    GtkWidget *install_btn = gtk_button_new_with_label("Install Now");
    GtkStyleContext *btn_style = gtk_widget_get_style_context(install_btn);
    gtk_style_context_add_class(btn_style, "suggested-action");

    g_signal_connect(install_btn, "clicked", G_CALLBACK(on_install_clicked), context);
    gtk_box_pack_start(GTK_BOX(vbox), install_btn, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    gtk_main();
}