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

#ifndef _FILE_QUEUE_H_
#define _FILE_QUEUE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib.h>

#include "file_multi.h"

/**
 * Structure holding a thread safe file queue.
 */
struct file_queue {
    GList *list; /**< List of files */
    GMutex list_mutex; /**< Lock for list of files */

    GAsyncQueue *queue; /**< Queue containing active files. */

    gint active; /**< Count of active objects. */
    GMutex active_mutex; /**< Lock for active count. */
    GCond active_cond; /**< Cond for active cout. */

    gboolean stop; /**< Stop flag */
};

extern struct file_queue *file_queue_new (guint refs);
extern void file_queue_free (struct file_queue *queue);

extern void file_queue_push (struct file_queue *queue, struct file_multi *file);
extern struct file_multi *file_queue_pop (struct file_queue *queue);
extern void file_queue_done (struct file_queue *queue);

extern GList *file_queue_get_list (struct file_queue *queue);

#endif /* _FILE_QUEUE_H_ */
