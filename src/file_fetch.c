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
 * Routines for threaded fetching of files.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>
#include <string.h>

#include "geh.h"
#include "file_multi.h"
#include "file_fetch.h"
#include "file_fetch_img.h"
#include "file_queue.h"
#include "thumb.h"
#include "ui_window.h"
#include "util.h"

#define IMAGE_EXT "bmp", "gif", "jpg", "jpeg", "png", "svg", "tiff", "xpm", NULL

static gpointer file_fetch_worker (gpointer data);
static void file_fetch_file (gpointer data, gpointer user_data);
static guint file_fetch_enqueue_images (struct file_fetch *file_fetch,
                                        GList *images);
static void file_fetch_progress (struct file_fetch *file_fetch,
                                 struct file_multi *file, gboolean status);

/**
 * Starts fetching of files.
 *
 * @param file_list GList of struct file_multi to fetch.
 * @return FALSE on error, else TRUE.
 */
struct file_fetch*
file_fetch_start (struct file_queue *queue, GList *file_list,
                  struct ui_window *ui)
{
    struct file_fetch *file_fetch;

    file_fetch = g_malloc (sizeof (struct file_fetch));

    file_fetch->ui = ui;
    file_fetch->queue = queue;

    /* Create hash table for keeping track of fetched files */
    file_fetch->hash = g_hash_table_new (g_str_hash, g_str_equal);
    g_mutex_init(&file_fetch->hash_mutex);

    file_fetch->stop = FALSE;

    /* Start worker thread which starts thread pool */
    file_fetch->thread =
        g_thread_new ("file_fetch_worker", (GThreadFunc) &file_fetch_worker, file_fetch);

    return file_fetch;
}

/**
 * Stop all fetching work.
 *
 * @param file_fetch struct file_fetch to stop.
 */
void
file_fetch_stop (struct file_fetch *file_fetch)
{
    g_assert (file_fetch);

    /* No locking, should be safe. */
    file_fetch->stop = TRUE;

    /* Join worker thread */
    g_thread_join (file_fetch->thread);

    /* Stop fetching threads */
    g_thread_pool_free (file_fetch->pool,
                        TRUE /* immediate */, TRUE /* wait */);

    /* Free resources */
    g_hash_table_destroy (file_fetch->hash);
    g_mutex_clear (&file_fetch->hash_mutex);
}

/**
 * Worked thread pushing files to thread pool.
 *
 * @param data Not used.
 * @return NULL.
 */
gpointer
file_fetch_worker (gpointer data)
{
    struct file_fetch *file_fetch = (struct file_fetch*) data;
    struct file_multi *file;

    g_assert (file_fetch);

    /* Create thread pool for fetching */
    file_fetch->pool = g_thread_pool_new ((GFunc) &file_fetch_file,
                                          data /* user data */,
                                          3 /* max threads */,
                                          FALSE /* exclusive */, NULL);

    /* Go through list of files and fetch */
    while (! file_fetch->stop
           && (file = file_queue_pop (file_fetch->queue)) != NULL) {
        if (file_multi_need_fetch (file)) {
            g_mutex_lock (&file_fetch->hash_mutex);
            if (! g_hash_table_lookup (file_fetch->hash,
                                       file_multi_get_path (file))) {
                /* Fetch file if needed */
                g_thread_pool_push (file_fetch->pool, (gpointer) file, NULL);
            }
            g_mutex_unlock (&file_fetch->hash_mutex);

        } else {
            /* Do not always use the thread pool as it might block if mixing
               files to fetch and files not needed to be fetched. */
            file_fetch_progress (file_fetch, file, TRUE);
            file_queue_done (file_fetch->queue);
        }
    }

    /* Hide progress bar when done */    
    ui_window_progress_hide (file_fetch->ui, TRUE /* lock */);

    return NULL;  
}

/**
 * Fetch next file in queue.
 *
 * @param data Pointer to struct file_multi.
 * @param user_data Not used.
 */
void
file_fetch_file (gpointer data, gpointer user_data)
{
    gboolean status;
    guint images_added, images_total, images_total_before;
    GList *images;

    /* Get file */
    struct file_fetch *file_fetch = (struct file_fetch*) user_data;
    struct file_multi *file = (struct file_multi*) data;

    /* Check total image count */
    images_total = ui_window_progress_get_total (file_fetch->ui);
    images_total_before = images_total;

    /* Double check fetched file hash to avoid race */
    g_mutex_lock (&file_fetch->hash_mutex);
    if (g_hash_table_lookup (file_fetch->hash, file_multi_get_path (file))) {
        file = NULL;
    } else {
        g_hash_table_insert (file_fetch->hash,
                             (gpointer) file_multi_get_path (file),
                             (gpointer) file);
    }
    g_mutex_unlock (&file_fetch->hash_mutex);

    /* File was already fetched, skip */
    if (file) {
        /* Fetch the file */
        status = file_multi_fetch (file, &file_fetch->stop);
        if (status) {
            /* Successfully fetched file */
            if (file_multi_get_ext (file)
                && util_str_in (file_multi_get_ext (file), TRUE /* casei */,
                                IMAGE_EXT)) {
                file_fetch_progress (file_fetch, file, status);

            } else {
                /* Extract image links from file (expected to be
                   non-binary) */
                images = file_fetch_img_extract_links (file);
                if (images) {
                    images_added = file_fetch_enqueue_images (file_fetch,
                                                              images);
                    images_total += images_added - 1;
                    g_list_free (images);

                } else {
                    images_total -= 1;
                }
            }
        } else {
            /* Failed to fetch, reduce number of files. */
            images_total -= 1;
        }
    }

    /* Set total number of images */
    if (images_total != images_total_before) {
        ui_window_progress_set_total (file_fetch->ui, images_total);
        ui_window_progress_progress (file_fetch->ui,
                                     0 /* count */, TRUE /* lock */);
    }

    file_queue_done (file_fetch->queue);
}

/**
 * Enqueue images on work queue, filter already fetched images first.
 *
 * @param file_fetch struct file_fetch to enqueu images for.
 * @param images GList of gchar URLs.
 * @return Number of images added to the queue.
 */
guint
file_fetch_enqueue_images (struct file_fetch *file_fetch, GList *images)
{
    GList *it;
    guint added = 0;

    g_assert (file_fetch);

    /* Lock hash table, enqueue go through the list of images and move all
       entries not in the hash table. */
    g_mutex_lock (&file_fetch->hash_mutex);
    for (it = images; it; it = it->next) {
        if (! g_hash_table_lookup (file_fetch->hash, (gchar*) it->data)) {
            file_queue_push (file_fetch->queue,
                             file_multi_open ((gchar*) it->data));
            added++;
        }
        g_free (it->data);
    }
    g_mutex_unlock (&file_fetch->hash_mutex);

    return added;
}


/**
 * Signals progress of fetched files.
 *
 * @param file_fetch File fetch to progress.
 * @param file Pointer to file_multi progressed.
 * @param status Status of the progress.
 */
void
file_fetch_progress (struct file_fetch *file_fetch,
                     struct file_multi *file, gboolean status)
{
    static gboolean first = TRUE;

    GdkPixbuf *thumb;

    if (first
        && (ui_window_get_mode (file_fetch->ui) != UI_WINDOW_MODE_THUMB)) {
        first = FALSE;

        /* Single file mode, set image */
        ui_window_set_image (file_fetch->ui, file,
                             file_fetch->ui->zoom_fit, TRUE /* lock */);
    }

    /* Always add thumbnail version so switching of modes is possible. */
    thumb =  thumb_get (file, options.thumb_side, TRUE);
    if (thumb) {
        ui_window_add_thumbnail (file_fetch->ui, file, thumb);
    }
    ui_window_progress_progress (file_fetch->ui,
                                 1 /* count */, TRUE /* lock */);
}
