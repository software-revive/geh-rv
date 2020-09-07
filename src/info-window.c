/**
 * Copyright (c) 2012-2016 Vadim Ushakov <igeekless@gmail.com>
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


#include "info-window.h"
#include <gdk/gdkkeysyms.h>

static gboolean on_info_window_key_press_event(
    GtkWidget * widget,
    GdkEventKey * event,
    gpointer user_data)
{
    if (event->keyval == GDK_Escape)
    {
        gtk_widget_hide (widget);
        gtk_widget_destroy (widget);
    }
    return FALSE;
}


GtkWidget * create_info_window (GtkWindow * parent, const char * title, const char * text)
{
    GtkWidget * info_window;
    GtkWidget * scrolled_window;
    GtkWidget * text_view;

    info_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (info_window, 600, 400);
    gtk_window_set_modal (GTK_WINDOW (info_window), TRUE);
    gtk_window_set_destroy_with_parent (GTK_WINDOW (info_window), TRUE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (info_window), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (info_window), TRUE);

    if (parent)
        gtk_window_set_transient_for (GTK_WINDOW (info_window), parent);

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolled_window);
    gtk_container_add (GTK_CONTAINER (info_window), scrolled_window);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    text_view = gtk_text_view_new ();
    gtk_widget_show (text_view);
    gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW (text_view), 10);
    gtk_text_view_set_right_margin (GTK_TEXT_VIEW (text_view), 10);

    g_signal_connect ((gpointer) info_window, "key_press_event",
                      G_CALLBACK (on_info_window_key_press_event),
                      NULL);


    PangoFontDescription * font_desc = pango_font_description_from_string ("Monospace");
    gtk_widget_modify_font (text_view, font_desc);
    pango_font_description_free (font_desc);

    GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
    gtk_text_buffer_set_text (buffer, text, -1);
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (text_view), buffer);
    g_object_unref (buffer);

    gtk_window_set_title (GTK_WINDOW (info_window), title);

    return info_window;
}
