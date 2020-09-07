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
 * Directory scanning routines.
 */

#ifndef _DIR_H_
#define _DIR_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib.h>

#include "file_queue.h"

/**
 * Dir scanner structure, holds thread info etc.
 */
struct dir_scan {
    struct file_queue *queue; /**< File queue for work thread. */
    gchar **files; /**< NULL terminated list of files/directories. */

    void (*file_count_inc)(gpointer, gint); /**< File count callback. */
    gpointer file_count_inc_data; /**< Data for count callback. */

    GThread *thread_work; /**< Worker thread */
    gboolean stop; /**< Stop flag */
};

extern struct dir_scan *dir_scan_start (struct file_queue *queue, gchar **files,
                                        void (*file_count_inc) (gpointer,
                                                                gint),
                                        gpointer file_count_inc_data);
extern void dir_scan_stop (struct dir_scan *ds);

#endif /* _DIR_H_ */
