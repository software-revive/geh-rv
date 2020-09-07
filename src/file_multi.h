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
 * Routines for opening files in multiple ways including remote
 * fetching.
 */

#ifndef _FILE_MULTI_H_
#define _FILE_MULTI_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib.h>

#include <sys/types.h>

#define FILE_MULTI_METHOD_PLAIN 1
#define FILE_MULTI_METHOD_STDIN 2
#define FILE_MULTI_METHOD_HTTP 3
#define FILE_MULTI_METHOD_FTP 4

/**
 * Main structure describing a multifile.
 */
struct file_multi {
    gchar *name; /**< (base)name of the file. */
    gchar *ext; /**< extenstion of the file. */
    gchar *uri; /**< URI to the file. */
    gchar *dir; /**< directory of the file. */
    gchar *path; /**< path to the file. */
    gchar *path_tmp; /**< path to the temporary storage of the file if any. */

    off_t size; /**< Size of file, -1 means not yet checked. */
    time_t mtime; /**< Mtime of file, -1 means not yet checked. */

    guint method; /**< Method needed for fetching the file. */
    gboolean need_fetch; /**< flag indicating if fetching is needed. */
};

extern struct file_multi *file_multi_open (const gchar *path);
extern void file_multi_close (struct file_multi *fm);
extern void file_multi_close_tmp (struct file_multi *fm);
extern gboolean file_multi_save (struct file_multi *fm, const gchar *path);
extern gboolean file_multi_rename (struct file_multi *fm, const gchar *name);

extern const gchar *file_multi_get_name (struct file_multi *fm);
extern const gchar *file_multi_get_ext (struct file_multi *fm);
extern const gchar *file_multi_get_uri (struct file_multi *fm);
extern const gchar *file_multi_get_dir (struct file_multi *fm);
extern const gchar *file_multi_get_path (struct file_multi *fm);

extern off_t file_multi_get_size (struct file_multi *fm);
extern time_t file_multi_get_mtime (struct file_multi *fm);

extern gboolean file_multi_fetch (struct file_multi *fm, gboolean *stop);
extern gboolean file_multi_need_fetch (struct file_multi *fm);

#endif /* _FILE_MULTI_H_ */
