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

#ifndef _FILE_FETCH_H_
#define _FILE_FETCH_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib.h>

#include "file_queue.h"
#include "ui_window.h"

/**
 * File fetch worker struct.
 */
struct file_fetch {
    struct ui_window *ui; /**< UI window to update. */
    struct file_queue *queue; /**< Queue to get work from and to. */

    GThread *thread; /**< Worker thread pushing files onto thread pool. */
    GThreadPool *pool; /**< Thread pool fetching files. */

    GHashTable *hash; /**< Hash table of fetched files. */
    GMutex hash_mutex; /**< Mutex for hash. */

    gboolean stop; /**< Stop flag. */
};

extern struct file_fetch *file_fetch_start (struct file_queue *queue,
                                            GList *file_list,
                                            struct ui_window *ui);

extern void file_fetch_stop (struct file_fetch *file_fetch);

#endif /* _FILE_FETCH_H_ */
