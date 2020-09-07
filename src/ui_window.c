/*
 * Copyright © 2006 Claes Nästén <me@pekdon.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * UI window handling code.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>

#include <string.h>
#include <stdlib.h>

#include "geh.h"
#include "ui_window.h"

/* Compatibility with older gtk+ versions */
#ifndef GDK_KEY_f
    #define GDK_KEY_0 GDK_0
    #define GDK_KEY_f GDK_f
    #define GDK_KEY_F GDK_F
    #define GDK_KEY_n GDK_n
    #define GDK_KEY_N GDK_N
    #define GDK_KEY_s GDK_s
    #define GDK_KEY_S GDK_S
    #define GDK_KEY_t GDK_t
    #define GDK_KEY_T GDK_T
    #define GDK_KEY_p GDK_p
    #define GDK_KEY_P GDK_P
    #define GDK_KEY_q GDK_q
    #define GDK_KEY_Q GDK_Q
    #define GDK_KEY_R GDK_R
    #define GDK_KEY_plus GDK_plus
    #define GDK_KEY_minus GDK_minus
    #define GDK_KEY_F11 GDK_F11
#endif

static GtkWidget *ui_window_create_menu (struct ui_window *ui);
static void ui_window_update_image (struct ui_window *ui);

/* Callbacks */
static gboolean callback_key_press (GtkWidget *widget,
                                    GdkEventKey *key, gpointer data);
static void callback_image (GtkIconView *icon_view, GtkTreePath *path, gpointer data);
static void callback_zoom (GtkWidget *widget, GdkEventScroll *event, gpointer user_Data);
static void callback_icon_edited (GtkCellRendererText *cell,
                                  gchar *path_string, gchar *text,
                                  gpointer data);

static gboolean idle_zoom_fit (gpointer data);

static gboolean callback_menu (GtkWidget *widget, GdkEvent *event);
static void callback_menu_zoom_orig (GtkMenuItem *item, gpointer data);
static void callback_menu_zoom_fit (GtkMenuItem *item, gpointer data);
static void callback_menu_zoom_in (GtkMenuItem *item, gpointer data);
static void callback_menu_zoom_out (GtkMenuItem *item, gpointer data);
static void callback_menu_rotate_left (GtkMenuItem *item, gpointer data);
static void callback_menu_rotate_right (GtkMenuItem *item, gpointer data);
static void callback_menu_file_save (GtkMenuItem *item, gpointer data);
static void callback_menu_file_rename (GtkMenuItem *item, gpointer data);

static void slide_next (struct ui_window *ui);
static void slide_prev (struct ui_window *ui);

/**
 * Create new ui_window.
 */
struct ui_window*
ui_window_new (void)
{
    GtkCellRenderer *icon_rend;
    struct ui_window *ui;

    ui = g_malloc (sizeof (struct ui_window));

    ui->is_fullscreen = FALSE;
    ui->width_alloc_prev = 0;
    ui->height_alloc_prev = 0;
    ui->thumbnails = 0;
    ui->file = NULL;
    ui->image_data = NULL;
    ui->progress_total = 0;
    ui->progress_step = 0.0;
    ui->icon_iter.stamp = 0;

    /* Create main UI window */
    ui->window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
    if (options.win_nodecor) {
        gtk_window_set_decorated (ui->window, FALSE);
    }
    if ((options.win_width > 0) && (options.win_height > 0)) {
        gtk_window_set_default_size (ui->window,
                                     options.win_width, options.win_height);
    }

    g_signal_connect (G_OBJECT (ui->window), "delete_event",
                      G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (G_OBJECT (ui->window), "key_press_event",
                      G_CALLBACK (callback_key_press), ui);

    /* Create vertical box */
#if GTK_CHECK_VERSION(3, 0, 0)
    ui->vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    ui->pane = GTK_PANED (gtk_paned_new (GTK_ORIENTATION_VERTICAL));
#else /* GTK < 3 */
    ui->vbox = GTK_BOX (gtk_vbox_new (FALSE, 0));
    ui->pane = GTK_PANED (gtk_vpaned_new ());
#endif

    /* Create image area */
    ui->image_window = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ui->image_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    /* Zoom and rotate signals on window */
    g_signal_connect (GTK_WIDGET (ui->image_window), "scroll-event",
                      G_CALLBACK (callback_zoom), ui);

    ui->image = GTK_IMAGE (gtk_image_new ());

    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (ui->image_window),
                                           GTK_WIDGET (ui->image));

    /* Create store for icon view */
    ui->icon_store = gtk_list_store_new (UI_ICON_STORE_FIELDS,
                                         G_TYPE_POINTER, /* struct file */
                                         G_TYPE_STRING, /* Display name */
                                         GDK_TYPE_PIXBUF); /* Thumbnail */

    /* Create thumbnail area */
    ui->icon_view_window = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ui->icon_view_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    ui->icon_view = GTK_ICON_VIEW (gtk_icon_view_new_with_model (GTK_TREE_MODEL (ui->icon_store)));

    /* Map fields to view */
    gtk_icon_view_set_item_width (ui->icon_view, options.thumb_side + UI_THUMB_PADDING);
    gtk_icon_view_set_pixbuf_column (ui->icon_view, UI_ICON_STORE_THUMB);

    /* Make names editable */
    icon_rend = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (ui->icon_view), icon_rend, TRUE);
    g_object_set (icon_rend, "editable", FALSE, NULL);
    g_signal_connect (icon_rend, "edited",
                      G_CALLBACK (callback_icon_edited), ui);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (ui->icon_view), icon_rend,
                                    "text", UI_ICON_STORE_NAME, NULL);

    g_signal_connect (ui->icon_view, "item_activated",
                      G_CALLBACK (callback_image), ui);

    gtk_container_add (GTK_CONTAINER (ui->icon_view_window),
                       GTK_WIDGET (ui->icon_view));

    /* Fill pane */
    gtk_paned_pack1 (ui->pane, GTK_WIDGET (ui->image_window),
                     TRUE /* resize */, TRUE /* shrink */);

    gtk_paned_pack2 (ui->pane, GTK_WIDGET (ui->icon_view_window),
                     TRUE /* resize */, TRUE /* shrink */);

    /* Create progress */
    ui->progress = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
    g_object_ref (ui->progress);

    /* Fill vbox */
    gtk_box_pack_start (GTK_BOX (ui->vbox), GTK_WIDGET (ui->pane),
                        TRUE, TRUE, 0);
    if (! options.win_nodecor) {
        ui_window_progress_show (ui, FALSE /* lock */);
    }

    /* Fill window */
    gtk_container_add (GTK_CONTAINER (ui->window), GTK_WIDGET (ui->vbox));

    /* connect our handler which will pop-up the menu */
    g_signal_connect_swapped (ui->window, "button_press_event",
                              G_CALLBACK (callback_menu),
                              ui_window_create_menu (ui));

    return ui;
}

/**
 * Frees resources used by struct ui_window.
 *
 * @param ui Pointer to struct ui_window to free.
 */
void
ui_window_free (struct ui_window *ui)
{
    g_assert (ui);

    /* Unref explicitly ref widgets */
    g_object_unref (ui->icon_store);
    g_object_unref (ui->progress);

    if (ui->image_data) {
        image_close (ui->image_data);
    }

    g_free (ui);
}

/**
 * Shows ui window.
 *
 * @param ui Pointer to struct ui_window to show.
 */
void
ui_window_show (struct ui_window *ui)
{
    g_assert (ui);

    gtk_widget_show_all (GTK_WIDGET (ui->window));
}

/**
 * Returns the current mode of the window.
 *
 * @param ui Pointer to struct ui_window.
 * @return Returns the current mode of the window.
 */
guint
ui_window_get_mode (struct ui_window *ui)
{
    g_assert (ui);

    return ui->mode;
}

/**
 * Activates specified mode for window.
 *
 * @param im Pointer to struct ui_window to set mode on.
 * @param mode Mode to set.
 */
void
ui_window_set_mode (struct ui_window *ui, guint mode)
{
    gint max_pos, pane_pos = 0;
    GList *selection;
    GtkTreePath *tree_path;

    g_assert (ui);

    /* Get window height, used when calculate the position of the pane */
    g_object_get(ui->pane, "max-position", &max_pos, NULL);

    if (mode == UI_WINDOW_MODE_FULL) {
        /* Full-screen mode, hiding the icon view completely */
        pane_pos = max_pos;

    } else if (mode == UI_WINDOW_MODE_SLIDE) {
        /* Slide-show mode, having a thumbnail height bottom border
           with icons */
        pane_pos = max_pos - options.thumb_side - UI_SLIDE_PADDING;
        /* Set columns to display in thumbnail mode */
        gtk_icon_view_set_columns (ui->icon_view, ui->thumbnails); 

    } else if (mode == UI_WINDOW_MODE_THUMB) {
        /* Set columns to display in thumbnail mode */
        gtk_icon_view_set_columns (ui->icon_view, -1);
    }

    gtk_paned_set_position (ui->pane, pane_pos);

    /* Re-focus thumbnail image to currently displayed image if any. */
    selection = gtk_icon_view_get_selected_items(ui->icon_view);
    if (selection != NULL) {
        if (selection->data != NULL) {
            /* For some reason scroll_to_path has to be called twice to
               work on my setup. */
            tree_path = (GtkTreePath*) selection->data;
            gtk_icon_view_scroll_to_path (ui->icon_view, tree_path, TRUE, 0.5, 0.5);
            gtk_icon_view_scroll_to_path (ui->icon_view, tree_path, TRUE, 0.5, 0.5);
        }
        g_list_foreach (selection, (GFunc)gtk_tree_path_free, NULL);
        g_list_free (selection);
    }

    /* Store mode */
    ui->mode = mode;
}

/**
 * Sets the file to be used as the current image.
 *
 * @param ui Pointer to struct ui_window.
 * @param file Struct file_multi to get image data from.
 * @param zoom_fit Zoom image to fit when displaying.
 * @param lock Lock GDK.
 */
void
ui_window_set_image (struct ui_window *ui, struct file_multi *file,
                     gboolean zoom_fit, gboolean lock)
{
    gchar *title;
    struct image *image_old = ui->image_data;

    g_assert (ui);

    /* Thread safety */
    if (lock) {
        gdk_threads_enter ();
    }

    /* Update title */
    title = g_strdup_printf ("geh: %s", file_multi_get_name (file));
    gtk_window_set_title (ui->window, title);
    g_free (title);

    /* Open new image */
    ui->file = file;
    ui->image_data = image_open (file_multi_get_path (file));
    if (ui->image_data) {
        if (zoom_fit) {
            /* Use an idle function so that the UI gets to update
               its size before zooming to fit. */
            g_idle_add (&idle_zoom_fit, (void*) ui);
        } else {
            /* Display file */
            ui_window_update_image (ui);
        }
    } else {
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
               "Failed to activate image %s", file_multi_get_path (file));
    }

    /* Clean up resources */
    if (image_old) {
        image_close (image_old);
    }

    if (lock) {
        gdk_threads_leave ();
    }
}

/**
 * Adds thumbnail to thumbnail view.
 *
 * @param ui Pointer to struct ui_window.
 * @param path Pointer to original file.
 * @param pix Pointer to GdkPixbuf to add.
 */
void
ui_window_add_thumbnail (struct ui_window *ui, struct file_multi *file, GdkPixbuf *pix)
{
    g_assert (ui);

    /* Thread safety */
    gdk_threads_enter ();

    /* Set correct amount of columns */
    ui->thumbnails++;
    if (ui->mode != UI_WINDOW_MODE_THUMB) {
        gtk_icon_view_set_columns (ui->icon_view, ui->thumbnails);
    }


    /* Limit length of name. */
    gchar *name = g_strdup (file_multi_get_name (file));
    if (g_utf8_validate (name, -1, NULL)) {
        if (g_utf8_strlen (name, -1) > UI_THUMB_CHARS) {
            int i;
            gchar *p = name;
            for (i = UI_THUMB_CHARS - 3; i > 0; i--) {
                p = g_utf8_next_char (p);
            }
            g_sprintf (p, "...");
        }
    } else if (strlen (name) > UI_THUMB_CHARS) {
        g_sprintf (name + UI_THUMB_CHARS - 4, "...");
    }

    /* Add thumbnail */
    gtk_list_store_append (ui->icon_store, &ui->icon_iter_add);
    gtk_list_store_set (ui->icon_store, &ui->icon_iter_add,
                        UI_ICON_STORE_FILE, file,
                        UI_ICON_STORE_NAME, name,
                        UI_ICON_STORE_THUMB, pix, -1);

    gdk_threads_leave ();

    free (name);
}

/**
 * Builds main menu for geh.
 *
 * @param ui Pointer to struct ui_window to build menu for.
 * @return GtkWidget representing menu.
 */
GtkWidget*
ui_window_create_menu (struct ui_window *ui)
{
    GtkWidget *menu, *menu_file;
    GtkWidget *item;

    /* Create menu */
    menu = gtk_menu_new ();

    /* Zoom Original */
    item = gtk_menu_item_new_with_label ("Zoom Orig");
    g_signal_connect (item, "activate",
                      G_CALLBACK (callback_menu_zoom_orig), ui);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);


    /* Zoom fit */
    item = gtk_menu_item_new_with_label ("Zoom Fit");
    g_signal_connect (item, "activate",
                      G_CALLBACK (callback_menu_zoom_fit), ui);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    /* Zoom in */
    item = gtk_menu_item_new_with_label ("Zoom In");
    g_signal_connect (item, "activate",
                      G_CALLBACK (callback_menu_zoom_in), ui);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    /* Zoom out */
    item = gtk_menu_item_new_with_label ("Zoom Out");
    g_signal_connect (item, "activate",
                      G_CALLBACK (callback_menu_zoom_out), ui);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    /* Separator */
    gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                           gtk_separator_menu_item_new ());

    /* Rotate left */
    item = gtk_menu_item_new_with_label ("Rotate Left");
    g_signal_connect (item, "activate",
                      G_CALLBACK (callback_menu_rotate_left), ui);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    /* Rotate right */
    item = gtk_menu_item_new_with_label ("Rotate Right");
    g_signal_connect (item, "activate",
                      G_CALLBACK (callback_menu_rotate_right), ui);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    /* Separator */
    gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                           gtk_separator_menu_item_new ());

    /* Create file operation menu */
    menu_file = gtk_menu_new ();

    item = gtk_menu_item_new_with_label ("Rename");
    g_signal_connect (item, "activate",
                      G_CALLBACK (callback_menu_file_rename), ui);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_file), item);

    item = gtk_menu_item_new_with_label ("Save");
    g_signal_connect (item, "activate",
                      G_CALLBACK (callback_menu_file_save), ui);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_file), item);

    item = gtk_menu_item_new_with_label ("File");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu_file);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    /* Separator */
    gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                           gtk_separator_menu_item_new ());

    /* Quit */
    item = gtk_menu_item_new_with_label ("Quit");
    g_signal_connect (item, "activate", G_CALLBACK (gtk_main_quit), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    /* Show widgets */
    gtk_widget_show_all (menu);

    return menu;
}

/**
 * Updates image displayed with current image.
 *
 * @param ui Pointer to struct ui_window.
 */
void
ui_window_update_image (struct ui_window *ui)
{
    gtk_image_set_from_pixbuf (GTK_IMAGE (ui->image),
                               image_get_curr (ui->image_data));
}

gboolean
idle_zoom_fit (gpointer data)
{
    callback_menu_zoom_fit (NULL, data);
    return FALSE;
}

/**
 * Callback to handle key press events.
 *
 * @param widget Widget press was made on.
 * @param key Key Event generated execution of callback.
 * @param data Pointer to struct ui_window.
 */
gboolean
callback_key_press (GtkWidget *widget, GdkEventKey *key, gpointer data)
{
    struct ui_window *ui = (struct ui_window*) data;

    switch (key->keyval) {
    case GDK_KEY_0:
        callback_menu_zoom_orig (NULL, ui);
        break;
    case GDK_KEY_f:
        callback_menu_zoom_fit (NULL, ui);
        break;
    case GDK_KEY_F: /* Full mode */
        ui_window_set_mode (ui, UI_WINDOW_MODE_FULL);
        break;
    case GDK_KEY_s: /* Slide mode */
        ui_window_set_mode (ui, UI_WINDOW_MODE_SLIDE);
        break;
    case GDK_KEY_S:
        callback_menu_file_save (NULL, ui);
        break;
    case GDK_KEY_R:
        callback_menu_file_rename (NULL, ui);
        break;
    case GDK_KEY_t: /* Thumb mode */
    case GDK_KEY_T:
        ui_window_set_mode (ui, UI_WINDOW_MODE_THUMB);
        break;
    case GDK_KEY_q: /* Quit */
    case GDK_KEY_Q:
        gtk_main_quit ();
        break;
    case GDK_KEY_n:
    case GDK_KEY_N:
        /* Next image (if in slideshow/full mode). */
        slide_next (ui);
        break;
    case GDK_KEY_p:
    case GDK_KEY_P:
        /* Prev image (if in slideshow/full mode). */
        slide_prev (ui);      
        break;
    case GDK_KEY_minus:
        image_zoom (ui->image_data, -10);
        ui_window_update_image (ui);
        break;
    case GDK_KEY_plus:
        image_zoom (ui->image_data, 10);
        ui_window_update_image (ui);
        break;
    case GDK_KEY_F11:
        if (ui->is_fullscreen) {
            gtk_window_unfullscreen (ui->window);            
        } else {
            gtk_window_fullscreen (ui->window);
        }
        ui->is_fullscreen = ui->is_fullscreen == TRUE ? FALSE : TRUE;
        break;
    default:
        return FALSE;
        break;
    }

    return TRUE;
}

/**
 * Activates image.
 *
 * @param icon_view Icon View.
 * @param tree_path Tree Path.
 * @param data Data.
 */
void
callback_image (GtkIconView *icon_view, GtkTreePath *tree_path, gpointer data)
{
    struct ui_window *ui = (struct ui_window*) data;
    struct file_multi *file;

    /* Expand from thumb mode to slide mode. */
    if (ui->mode == UI_WINDOW_MODE_THUMB) {
        ui_window_set_mode (ui, UI_WINDOW_MODE_SLIDE);
    }

    /* Get current object */
    gtk_tree_model_get_iter (GTK_TREE_MODEL (ui->icon_store),
                             &ui->icon_iter, tree_path);
    gtk_tree_model_get (GTK_TREE_MODEL (ui->icon_store), &ui->icon_iter,
                        UI_ICON_STORE_FILE, &file, -1);
    gtk_icon_view_scroll_to_path (ui->icon_view, tree_path, FALSE, 0, 0);

    /* Activate image and ensure that thumbnail being visible */
    ui_window_set_image (ui, file, ui->zoom_fit, FALSE);
}

/**
 * Zooms image.
 *
 * @param widget Not used
 * @param event Event caused zooming.
 * @param user_data User data, pointer to ui.
 */
void
callback_zoom (GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
    struct ui_window *ui = (struct ui_window*) user_data;

    /* Nothing to do as there is no image */
    if (! ui->image_data) {
        return;
    }

    if (event->direction == GDK_SCROLL_UP) {
        /* Zoom image */
        image_zoom (ui->image_data, 10);

        /* Update image displayed */
        ui_window_update_image (ui);
    } else if (event->direction == GDK_SCROLL_DOWN) {
        /* Zoom image */
        image_zoom (ui->image_data, -10);

        /* Update image displayed */
        ui_window_update_image (ui);
    }
}

/**
 * Callback when text is edited for thumbnails, renames the file.
 *
 * @param cell Rendered generating the singnal.
 * @param path_string Path to item.
 * @param text Edited text.
 * @param data Pointer to struct ui_window.
 */
void
callback_icon_edited (GtkCellRendererText *cell,
                      gchar *path_string, gchar *text, gpointer data)
{
    struct ui_window *ui = (struct ui_window*) data;
    struct file_multi *file;

    GtkTreeIter iter;

    /* Nothing to do */
    if (! g_utf8_strlen (text, 2)) {
        return;
    }

    gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (ui->icon_store),
                                         &iter, path_string);
    gtk_tree_model_get (GTK_TREE_MODEL (ui->icon_store),
                        &iter, UI_ICON_STORE_FILE, &file, -1);

    if (file_multi_rename (file, text)) {
        gtk_list_store_set (GTK_LIST_STORE (ui->icon_store), &iter,
                            UI_ICON_STORE_NAME, text, -1);

    } else {
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
               "failed to rename file from %s to %s",
               file_multi_get_name (file), text);
    }
}

/**
 * Handles callbacks for displaying the menu.
 *
 * @param widget GtkWidget generating callback.
 * @param event GdkEvent for event.
 * @return TRUE if event is handled, else FALSE.
 */
gboolean
callback_menu (GtkWidget *widget, GdkEvent *event)
{
    GtkMenu *menu;
    GdkEventButton *event_button;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_MENU (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    /* Get the menu */
    menu = GTK_MENU (widget);

    if (event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;
        if (event_button->button == 3) {
            gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
                            event_button->button, event_button->time);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Zooms image to its original size.
 *
 * @param item Menu item that was activated.
 * @param data void pointer to struct ui_window.
 */
void
callback_menu_zoom_orig (GtkMenuItem *item, gpointer data)
{
    struct ui_window *ui = (struct ui_window*) data;

    /* Nothing to do as there is no image */
    if (! ui->image_data) {
        return;
    }

    /* Set zoom to available size */
    image_zoom_set (ui->image_data, 100);

    /* Update image displayed */
    ui_window_update_image (ui);
}


/**
 * Zooms image to fit window.
 *
 * @param item Menu item that was activated.
 * @param data void pointer to struct ui_window.
 */
void
callback_menu_zoom_fit (GtkMenuItem *item, gpointer data)
{
    GtkAllocation allocation;
    struct ui_window *ui = (struct ui_window*) data;

    /* Nothing to do as there is no image */
    if (! ui->image_data) {
        return;
    }

    /* Set zoom to available size */
    gtk_widget_get_allocation(GTK_WIDGET (ui->image_window), &allocation);
    image_zoom_fit (ui->image_data, allocation.width - 16, allocation.height - 16);

    /* Update image displayed */
    ui_window_update_image (ui);
}

/**
 * Zooms image in with 10 percent
 *
 * @param item
 * @param data
 */
void
callback_menu_zoom_in (GtkMenuItem *item, gpointer data)
{
    struct ui_window *ui = (struct ui_window*) data;

    /* Nothing to do as there is no image */
    if (! ui->image_data) {
        return;
    }

    /* Zoom */
    image_zoom (ui->image_data, 10);

    /* Update image displayed */
    ui_window_update_image (ui);
}

/**
 * Zooms image out with 10 percent
 *
 * @param item
 * @param data
 */
void
callback_menu_zoom_out (GtkMenuItem *item, gpointer data)
{
    struct ui_window *ui = (struct ui_window*) data;

    /* Nothing to do as there is no image */
    if (! ui->image_data) {
        return;
    }

    /* Zoom */
    image_zoom (ui->image_data, -10);

    /* Update image displayed */
    ui_window_update_image (ui);
}

/**
 * Rotates image 90 degrees.
 *
 * @param item
 * @param data
 */
void
callback_menu_rotate_left (GtkMenuItem *item, gpointer data)
{
    struct ui_window *ui = (struct ui_window*) data;

    /* Nothing to do as there is no image */
    if (! ui->image_data) {
        return;
    }

    /* Rotate */
    image_rotate (ui->image_data, 90);

    /* Update image displayed */
    ui_window_update_image (ui);
}

/**
 * Rotates image -90 degrees.
 *
 * @param item
 * @param data
 */
void
callback_menu_rotate_right (GtkMenuItem *item, gpointer data)
{
    struct ui_window *ui = (struct ui_window*) data;

    /* Nothing to do as there is no image */
    if (! ui->image_data) {
        return;
    }

    /* Rotate */
    image_rotate (ui->image_data, -90);

    /* Update image displayed */
    ui_window_update_image (ui);
}

/**
 * Shows file save dialog and saves active image if any.
 *
 * @param item GtkMenuItem activated.
 * @param data Pointer to struct ui_window.
 */
void
callback_menu_file_save (GtkMenuItem *item, gpointer data)
{
    GtkWidget *dialog;
    struct ui_window *ui = (struct ui_window*) data;
    gchar *save_path;

    /* Nothing to do as there is no file */
    if (! ui->file) {
        return;
    }

    /* Create save dialog and set current name to selected file name */
    dialog = gtk_file_chooser_dialog_new ("Save File", ui->window,
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          "Cancel", GTK_RESPONSE_CANCEL,
                                          "Save", GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                    TRUE);

    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                       file_multi_get_name (ui->file));

    /* Run dialog and save if wanted */
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        save_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        if (! file_multi_save (ui->file, save_path)) {
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                   "Failed to save file to %s", save_path);
        }

        g_free (save_path);
    }

    gtk_widget_destroy (dialog);
}

/**
 * Shows file save dialog and saves active image if any.
 *
 * @param item GtkMenuItem generating callback.
 * @param data Pointer to struct ui_window.
 */
void
callback_menu_file_rename (GtkMenuItem *item, gpointer data)
{
    GtkWidget *dialog, *input;
    struct ui_window *ui = (struct ui_window*) data;

    /* Nothing to do as there is no file */
    if (! ui->file) {
        return;
    }

    /* Create rename dialog */
    dialog = gtk_dialog_new_with_buttons ("Rename File", ui->window,
                                          0 /* flags */,
                                          "Cancel", GTK_RESPONSE_CANCEL,
                                          "Ok", GTK_RESPONSE_ACCEPT,
                                          NULL);

    /* Add file name input */
    input = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (input), file_multi_get_name (ui->file));
    gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), input);

    gtk_widget_show_all (dialog);

    /* Run dialog and save if wanted */
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        file_multi_rename (ui->file, gtk_entry_get_text (GTK_ENTRY (input)));
    }

    gtk_widget_destroy (dialog);
}

/**
 * Show progress bar.
 *
 * @param ui Pointer to struct ui_window to show progress bar on.
 * @param lock Lock GDK.
 */
void
ui_window_progress_show (struct ui_window *ui, gboolean lock)
{
    g_assert (ui);

    if (lock) {
        gdk_threads_enter ();
    }

    gtk_box_pack_end (GTK_BOX (ui->vbox), GTK_WIDGET (ui->progress),
                      FALSE, FALSE, 0);

    if (lock) {
        gdk_threads_leave ();
    }
}

/**
 * Hide progress bar.
 *
 * @param ui Pointer to struct ui_window to hide progress bar on.
 * @param lock Lock GDK.
 */
void
ui_window_progress_hide (struct ui_window *ui, gboolean lock)
{
    g_assert (ui);

    if (lock) {
        gdk_threads_enter ();
    }

    gtk_container_remove (GTK_CONTAINER (ui->vbox), GTK_WIDGET (ui->progress));

    if (lock) {
        gdk_threads_leave ();
    }
}

/**
 * Updates progress status with one item.
 *
 * @param ui struct ui_window to progress.
 * @param count Count progress, valid with both positive and negative numbers.
 * @param lock Lock GDK (when accessing from other thread)
 */
void
ui_window_progress_progress (struct ui_window *ui, gint count, gboolean lock)
{
    gdouble fraction = 1.0;

    g_assert (ui);

    if (lock) {
        gdk_threads_enter ();
    }

    /* Get progress */
    ui->progress_curr += count;
    if (ui->progress_curr < 0) {
        ui->progress_curr = 0;
    } else if (ui->progress_curr > ui->progress_total) {
        ui->progress_curr = ui->progress_total;
    }

    fraction = ui->progress_curr * ui->progress_step;

    gtk_progress_bar_set_fraction (ui->progress, fraction);

    if (lock) {
        gdk_threads_leave ();
    }
}

/**
 * Gets the total number of items to progress.
 *
 * @param  ui struct ui_window to get total number of items in.
 * @return Number of items to progress.
 */
guint
ui_window_progress_get_total (struct ui_window *ui)
{
    g_assert (ui);

    return ui->progress_total;
}

/**
 * Sets total number of items.
 *
 * @param ui struct ui_window to set total number of items on.
 * @param total Total number of items.
 */
void
ui_window_progress_set_total (struct ui_window *ui, guint total)
{
    g_assert (ui);

    /* Avoid division by zero */
    if (total == 0) {
        total = 1;
    }

    ui->progress_total = total;
    ui->progress_step = 1.0 / (gdouble) total;
}

/**
 * Adds count items to total count of items.
 *
 * @param data Pointer to struct ui_window.
 * @param count Number of items to add (- allowed)
 */
void
ui_window_progress_add (gpointer data, gint count)
{
    struct ui_window *ui = (struct ui_window*) data;

    /* Make sure count is not overflowed */
    if (count < 0 && abs (count) > ui->progress_total) {
        count = ui->progress_total;
    }

    ui_window_progress_set_total (ui, ui->progress_total + count);
}

/**
 * Show the next image in the set.
 */
void
slide_next (struct ui_window *ui)
{
    gboolean set_image = TRUE;
    struct file_multi *file;

    if (ui->icon_iter.stamp == 0) {
        set_image = FALSE;
    } else {
        set_image = gtk_tree_model_iter_next (
            GTK_TREE_MODEL (ui->icon_store), &ui->icon_iter);
    }

    if (! set_image) {
        set_image = gtk_tree_model_get_iter_first(
            GTK_TREE_MODEL (ui->icon_store), &ui->icon_iter);
    }

    if (set_image) {
        gtk_tree_model_get (GTK_TREE_MODEL (ui->icon_store), &ui->icon_iter,
                            UI_ICON_STORE_FILE, &file, -1);

        /* Update selected item. */
        GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (ui->icon_store), &ui->icon_iter);
        gtk_icon_view_select_path (ui->icon_view, path);
        gtk_icon_view_scroll_to_path (ui->icon_view, path, FALSE, 0, 0);
        gtk_tree_path_free (path);

        ui_window_set_image (ui, file, ui->zoom_fit, FALSE);        
    }
}

/**
 * Show the previous image in the set.
 */
void
slide_prev (struct ui_window *ui)
{
    int i;
    gboolean set_image;
    struct file_multi *file;
    GtkTreePath *path;

    if (ui->icon_iter.stamp == 0) {
        set_image = FALSE;
    } else {
        path = gtk_tree_model_get_path (GTK_TREE_MODEL (ui->icon_store), 
                                        &ui->icon_iter);
        set_image = gtk_tree_path_prev (path)
            && gtk_tree_model_get_iter (GTK_TREE_MODEL (ui->icon_store), 
                                        &ui->icon_iter, path);
        gtk_tree_path_free(path);
    }

    if (! set_image) {
        set_image = gtk_tree_model_get_iter_first(
            GTK_TREE_MODEL (ui->icon_store), &ui->icon_iter);        

        for (i = 1; i < ui->thumbnails && set_image; i++ ) {
            set_image = gtk_tree_model_iter_next (
                GTK_TREE_MODEL (ui->icon_store), &ui->icon_iter);
        }
    }

    if (set_image) {
        gtk_tree_model_get (GTK_TREE_MODEL (ui->icon_store), &ui->icon_iter,
                            UI_ICON_STORE_FILE, &file, -1);

        /* Update selected item. */
        GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (ui->icon_store), &ui->icon_iter);
        gtk_icon_view_select_path (ui->icon_view, path);
        gtk_icon_view_scroll_to_path (ui->icon_view, path, FALSE, 0, 0);
        gtk_tree_path_free (path);

        ui_window_set_image (ui, file, ui->zoom_fit, FALSE);        
    }
}
