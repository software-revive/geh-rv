/*
 * Copyright © 2006-2009 Claes Nästén <me@pekdon.net>
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
 * Main header, common data structures such as options.
 */

#ifndef _GEH_H_
#define _GEH_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>

/**
 * Option structure containing global options.
 */
struct _options {
    GList *file_list; /**< List of files to display. */

    guint mode; /**< Mode internal representation. */
    gchar *mode_str; /**< Mode to start geh in. */
    guint timeout; /**< Time to display geh. */

    gboolean win_nodecor; /**< Decor on window. */
    gint win_width; /**< Width of window. */
    gint win_height; /**< Width of window. */

    gboolean keep_size; /**< If true, do not zoom image to fit when changing. */
    guint thumb_side; /**< Maximum size of thumbnail in pixels. */

    gboolean recursive; /**< Recursive directory scanning. */
    guint levels; /**< Level of recursion. */

    gchar **files; /**< List containing all files given as arguments. */
};

extern struct _options options; /**< Global options. */

#endif /* _GEH_H_ */
