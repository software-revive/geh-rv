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

/*
 * Directory scanning routines.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "geh.h"
#include "dir.h"
#include "file_multi.h"
#include "file_queue.h"

static void dir_scan_worker (gpointer data);
static void dir_scan_recursive (struct dir_scan *ds,
                                const gchar *path, gint levels);

/**
 * Starts directory scanning thread.
 *
 * @param queue file_queue to push scanned files onto.
 * @param files NULL terminated list of files.
 * @param file_count_inc File count callback.
 * @param file_count_inc_data File count callback data.
 * @return Pointer to struct dir_scan doing the work.
 */
struct dir_scan*
dir_scan_start (struct file_queue *queue, gchar **files,
                void (*file_count_inc)(gpointer, gint),
                gpointer file_count_inc_data)
{
    struct dir_scan *ds;

    g_assert (files);

    /* Allocate dir scanner object. */
    ds = g_malloc (sizeof (struct dir_scan));
    g_assert (ds);

    /* Init */
    ds->queue = queue;
    ds->files = files;
    ds->file_count_inc = file_count_inc;
    ds->file_count_inc_data = file_count_inc_data;
    ds->stop = FALSE;

    /* Start worker thread scanning directories and files */
    ds->thread_work =
        g_thread_new ("dir_scan_worker", (GThreadFunc) &dir_scan_worker, ds);

    return ds;
}

/**
 * Stops directory scanning and frees resources.
 *
 * @param ds struct dir_scan to stop and free.
 */
void
dir_scan_stop (struct dir_scan *ds)
{
    g_assert (ds);

    /* No locking, should be safe. */
    ds->stop = TRUE;

    g_thread_join (ds->thread_work);

    /* Free resources */
    g_free (ds);
}

/**
 * Worker thread scanning directories.
 *
 * @param data Pointer to struct dir_scan doing the work.
 */
void
dir_scan_worker (gpointer data)
{
    gint i;
    struct dir_scan *ds = (struct dir_scan*) data;

    for (i = 0; ! ds->stop && ds->files[i] != NULL; i++) {
        if (g_file_test (ds->files[i], G_FILE_TEST_IS_DIR)) {
            if (options.recursive) {
                dir_scan_recursive (ds, ds->files[i], options.levels);
            }
        } else {
            file_queue_push (ds->queue, file_multi_open (ds->files[i]));
        }
    }

    /* Signal directory scanning done */
    file_queue_done (ds->queue);
}

/**
 * Scans path recursive going max levels down
 *
 * @param ds struct dir_scan to append on.
 * @param path Path to start scanning
 * @param levels max levels down, -1 is limitless
 */
void
dir_scan_recursive (struct dir_scan *ds, const gchar *path, gint levels)
{
    gchar *file;
    guint added = 0;
    const gchar *name;
    GDir *dir;
    GList *list = NULL, *it;

    /* Check that path is a directory */
    if (! g_file_test (path, G_FILE_TEST_IS_DIR)) {
        g_warning ("%s is not a valid directory", path);
        return;
    }

    /* Open directory */
    dir = g_dir_open (path, 0, NULL);
    if (!dir) {
        g_warning ("unable to open %s as directory", path);
        return;
    }

    /* Scan directory, store files for sorting later on. */
    while (! ds->stop && (name = g_dir_read_name (dir)) != NULL) {
        file = g_build_filename (path, name, NULL);
        if (g_file_test (file, G_FILE_TEST_IS_DIR)) {
            dir_scan_recursive (ds, file,
                                (levels == -1) ? levels : levels - 1);
        } else {
            list = g_list_insert_sorted (list, (gpointer) file,
                                         (GCompareFunc) &strcmp);
        }
    }
    g_dir_close (dir);

    /* Scan files. */
    for (it = g_list_first (list); it != NULL; it = g_list_next (it)) {
        name = (const char*) it->data;
        if (g_file_test (name, G_FILE_TEST_IS_REGULAR)) {
            file_queue_push (ds->queue, file_multi_open (name));
            added++;
        }
    }

    /* Add to total number of items (progress bar) */
    if (added > 0) {
        ds->file_count_inc (ds->file_count_inc_data, added);
    }

    /* Cleanup */
    for (it = g_list_first (list); it != NULL; it = g_list_next (it)) {
        free (it->data);
    }
    g_list_free (list);
}
