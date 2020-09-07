/**
 * Copyright (c) 2020 Vadim Ushakov <igeekless@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>

#include "about.h"

/* FIXME: */
#define _(x) (x)

gchar * get_about_message(void)
{
    const char * divider =
        "\n------------------------------------------------------\n\n";

    GString * text = g_string_new ("");

    g_string_append (text, _(
        "Geh is a simple image viewer written in C/Gtk+\n"
    ));

    g_string_append_printf (text, _("Version: %s\n"), VERSION);

    g_string_append (text, divider);

    /* FIXME: */
    /*g_string_append_printf (text, _("Homepage: %s\n"), "");
    g_string_append_printf (text, _("You can take the latest sources at %s\n"), "");
    g_string_append_printf (text, _("Please report bugs and feature requests to %s.\n"), "");

    g_string_append (text, divider);*/

    g_string_append (text, _(
        "Copyright (C) 2020 Vadim Ushakov <igeekless@gmail.com>\n"
        "Copyright (C) 2006-2009 Claes Nästén <me@pekdon.net>\n"
    ));

    g_string_append (text, divider);

    g_string_append (text, _(
        "Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions\n\n"

        "The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n\n"

    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."
    ));

#if 0
    /* TRANSLATORS: Replace this string with your names, one name per line. */
    const char * translator_credits = _( "translator-credits" );
    if (g_strcmp0(_( "translator-credits" ), "translator-credits") != 0)
    {
        g_string_append (text, divider);

        g_string_append (text, _(
            "Translators:\n\n"
        ));

        g_string_append (text, translator_credits);

        if (translator_credits[strlen(translator_credits)] != '\n')
            g_string_append ( text, "\n");
    }
#endif
    g_string_append (text, divider);

    g_string_append (text, _(
        "Supported Image Formats:\n\n"
    ));


    GSList* formats = gdk_pixbuf_get_formats ();
    GSList* format_entry;
    for (format_entry = formats; format_entry != NULL; format_entry = format_entry->next )
    {
        GdkPixbufFormat * format = (GdkPixbufFormat *) format_entry->data;

        if (gdk_pixbuf_format_is_disabled (format))
            continue;

        gchar * format_name = gdk_pixbuf_format_get_name (format);
        gchar * format_description = gdk_pixbuf_format_get_description (format);
        gchar * format_license = gdk_pixbuf_format_get_license (format);
        gboolean format_writable = gdk_pixbuf_format_is_writable (format);
        gboolean format_scalable = gdk_pixbuf_format_is_scalable (format);
        gchar ** format_mime_types_v = gdk_pixbuf_format_get_mime_types (format);
        gchar ** format_extensions_v = gdk_pixbuf_format_get_extensions (format);
        gchar * format_mime_types = g_strjoinv (", ", format_mime_types_v);
        gchar * format_extensions = g_strjoinv (", ", format_extensions_v);


        g_string_append_printf (text, _(
            "Image loader: GdkPixBuf::%s (%s)\n"
            "    License:    %s\n"
            "    Writable:   %s\n"
            "    Scalable:   %s\n"
            "    MIME types: %s\n"
            "    Extensions: %s\n"
            "\n"),
            format_name, format_description,
            format_license,
            format_writable ? _("Yes") : _("No"),
            format_scalable ? _("Yes") : _("No"),
            format_mime_types,
            format_extensions);

        g_free (format_name);
        g_free (format_description);
        g_free (format_license);
        g_strfreev (format_mime_types_v);
        g_strfreev (format_extensions_v);
        g_free (format_mime_types);
        g_free (format_extensions);
    }

    g_string_append (text, divider);

    g_string_append (text, _(
        "Compile-time environment:\n"
    ));

    g_string_append_printf (text, "glib %d.%d.%d\n", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
    g_string_append_printf (text, "gtk %d.%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    g_string_append_printf (text, "Configured with: %s\n", CONFIGURE_ARGUMENTS);

    return g_string_free (text, FALSE);
}

gchar * get_key_bindings_message(void)
{
    return g_strdup(
        "Geh Key Bindings:\n\n"
        "f,   zoom to fit.\n"
        "F,   full mode showing only the current picture.\n"
        "s,   slide mode showing a large picture and one row of thumbnails.\n"
        "S,   open save picture dialog.\n"
        "R,   open rename picture dialog.\n"
        "t T, thumbnail mode showing only thumbnails.\n"
        "q Q, quit.\n"
        "n N, show/select next image.\n"
        "p P, show/select previous image.\n"
        "+,   zoom in current image.\n"
        "-,   zoom out current image.\n"
        "0,   zoom current image to original size.\n"
        "F11, toggle fullscreen mode.\n"
    );
}
