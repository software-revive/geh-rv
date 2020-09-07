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
 * File queue and list.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib.h>

#include "file_queue.h"

/**
 * Creates new struct file_queue.
 *
 * @param refs Number of references before done.
 * @return Pointer to newly created struct file_queue or NULL on error.
 */
struct file_queue*
file_queue_new (guint refs)
{
    struct file_queue *queue;

    queue = g_malloc (sizeof (struct file_queue));

    queue->list = NULL;
    g_mutex_init (&queue->list_mutex);
    queue->queue = g_async_queue_new ();

    queue->active = refs;
    g_mutex_init (&queue->active_mutex);
    g_cond_init (&queue->active_cond);

    queue->stop = FALSE;

    return queue;
}

/**
 * Frees resources used by queue.
 *
 * @param queue Pointer to struct file_queue to free.
 */
void
file_queue_free (struct file_queue *queue)
{
    g_assert (queue);

    if (queue->list) {
        g_list_free (queue->list);
    }
    g_mutex_clear (&queue->list_mutex);
    g_async_queue_unref (queue->queue);

    g_mutex_clear (&queue->active_mutex);
    g_cond_clear (&queue->active_cond);

    g_free (queue);
}

/**
 * Pushes file onto queue.
 *
 * @param queue struct file_queue to push file to.
 * @param file struct file_multi to push onto queue.
 */
void
file_queue_push (struct file_queue *queue, struct file_multi *file)
{
    g_assert (queue);

    /* Add file to list of known files */
    g_mutex_lock (&queue->list_mutex);
    queue->list = g_list_append (queue->list, file);
    g_mutex_unlock (&queue->list_mutex);

    /* Add active */
    g_mutex_lock (&queue->active_mutex);
    queue->active++;
    g_mutex_unlock (&queue->active_mutex);

    /* Push to work queue */
    g_async_queue_push (queue->queue, file);
}

/**
 * Pops file from queue.
 *
 * @param queue struct file_queue to pop file from.
 * @return Pointer to struct file_multi or NULL if no item left.
 * @todo Proper end of queue detection.
 */
struct file_multi*
file_queue_pop (struct file_queue *queue)
{
    struct file_multi *file = NULL;

    g_assert (queue);

    g_mutex_lock (&queue->active_mutex);
    file = (struct file_multi*) g_async_queue_try_pop (queue->queue);
    while (!file && queue->active) {
        g_cond_wait (&queue->active_cond, &queue->active_mutex);
        file = (struct file_multi*) g_async_queue_try_pop (queue->queue);
    }
    g_mutex_unlock (&queue->active_mutex);

    return file;
}

/**
 * Returns the list of files that has been in the queue.
 *
 * @return GList of struct file_multi that has been in the queue.
 */
GList*
file_queue_get_list (struct file_queue *queue)
{
    g_assert (queue);

    return queue->list;
}

/**
 * Signals object finished in queue.
 *
 * @param queue struct file_queue to signal finished.
 */
void
file_queue_done (struct file_queue *queue)
{
    g_assert (queue);

    g_mutex_lock (&queue->active_mutex);
    if (queue->active > 0) {
        queue->active--;
    }
    g_cond_signal (&queue->active_cond);
    g_mutex_unlock (&queue->active_mutex);
}
