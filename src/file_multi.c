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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib.h>
#include <glib/gstdio.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

#include "file_multi.h"
#include "util.h"

#define BUF_STDIN 8192
#define BUF_SAVE 8192

#define WGET_CHECK_INTERVAL 50000
#define WGET_DIE_SIGNAL 9
#define WGET_KILL_SIGNAL 15
#define WGET_KILL_WAIT 500000
#define WGET_SPAWN_FLAGS G_SPAWN_DO_NOT_REAP_CHILD|G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL

static void file_multi_free_strings (struct file_multi *fm);

static guint file_multi_get_method (const gchar *path);
static gchar *file_multi_create_name (const gchar *path, guint method);
static gchar *file_multi_create_ext (const gchar *path);
static gchar *file_multi_create_dir (const gchar *path);
static gchar *file_multi_create_uri (const gchar *path, guint method);

static gchar *file_multi_create_tmpname (void);

static gboolean file_multi_fetch_stdin (struct file_multi *fm,
                                        gboolean *stop);
static gboolean file_multi_fetch_http (struct file_multi *fm,
                                       gboolean *stop);
static gboolean file_multi_fetch_ftp (struct file_multi *fm,
                                      gboolean *stop);

static gboolean file_multi_fetch_wget (struct file_multi *fm,
                                       gboolean *stop);

/**
 * Open and create new struct file_multi.
 *
 * @param path Path to file that is to be opened.
 */
struct file_multi*
file_multi_open (const gchar *path)
{
    struct file_multi *fm;

    g_assert (path);

    /* Create new struct file_multi */
    fm = g_malloc (sizeof (struct file_multi));
    fm->name = NULL;
    fm->ext = NULL;
    fm->uri = NULL;
    fm->dir = NULL;
    fm->path = g_strdup (path);
    fm->path_tmp = NULL;
    fm->mtime = -1;
    fm->method = FILE_MULTI_METHOD_PLAIN;
    fm->need_fetch = FALSE;

    /* Identify method to fetch file with (if needed) */
    fm->method = file_multi_get_method (fm->path);
    if (fm->method != FILE_MULTI_METHOD_PLAIN) {
        fm->need_fetch = TRUE;
    }

    return fm;
}

/**
 * Close file and clean up temporary file if any.
 *
 * @param fm Pointer to struct file_multi
 */
void
file_multi_close (struct file_multi *fm)
{
    g_assert (fm);

    file_multi_free_strings (fm);
    file_multi_close_tmp (fm);

    g_free (fm);
}

/**
 * Clean up temporary file if any.
 *
 * @param fm Pointer to struct file_multi
 */
void
file_multi_close_tmp (struct file_multi *fm)
{
    g_assert (fm);

    /* Clean up temporary file if any. */
    if (fm->path_tmp) {
        g_unlink (fm->path_tmp);
        g_free (fm->path_tmp);
        fm->path_tmp = NULL;
    }
}

/**
 * Free resources used by parts of the name.
 *
 * @param fm struct file_multi to free resources in.
 */
void
file_multi_free_strings (struct file_multi *fm)
{
    if (fm->name) {
        g_free (fm->name);
        fm->name = NULL;
    }
    if (fm->ext) {
        g_free (fm->ext);
        fm->ext = NULL;
    }
    if (fm->uri) {
        g_free (fm->uri);
        fm->uri = NULL;
    }
    if (fm->dir) {
        g_free (fm->dir);
        fm->dir = NULL;
    }
    if (fm->path) {
        g_free (fm->path);
        fm->path = NULL;
    }
}

/**
 * Save file (or temporary file) to path.
 *
 * @param fm struct file_multi to save.
 * @param path Path to save file to.
 * @return TRUE on success, else FALSE.
 */
gboolean
file_multi_save (struct file_multi *fm, const gchar *path)
{
    gchar buf[BUF_SAVE];
    size_t buf_read;
    FILE *in, *out;

    /* Open input file */
    in = g_fopen (file_multi_get_path (fm), "rb");
    if (! in) {
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
               "Failed to open %s for reading.", file_multi_get_path (fm));
        return FALSE;
    }

    /* Create output file */
    out = g_fopen (path, "wb");
    if (! out) {
        fclose (in);

        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
               "Failed to open %s for writing.", path);
        return FALSE;
    }

    /* Copy in to out */
    while ((buf_read = fread (buf, sizeof (gchar), BUF_SAVE, in)) > 0) {
        fwrite (buf, sizeof (gchar), buf_read, out);
    }

    /* Close output */
    fclose (in);
    fclose (out);

    return TRUE;
}

/**
 * Renames file to new name in same directory.
 *
 * @param fm struct file_multi to rename.
 * @param name New name.
 * @return TRUE on success, else FALSE.
 */
gboolean
file_multi_rename (struct file_multi *fm, const gchar *name)
{
    gchar *path_new;
    gboolean status = FALSE;

    g_assert (fm);

    path_new = g_strjoin ("/", file_multi_get_dir (fm), name, NULL);

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
           "Rename %s to %s", file_multi_get_path (fm), path_new);
    if (! rename (file_multi_get_path (fm), path_new)) {
        status = TRUE;

        /* Free uri etc, as it gets changed by rename */
        file_multi_free_strings (fm);

        fm->path = path_new;
        if (fm->path_tmp) {
            /* Keep temporary file, update with new path */
            g_free (fm->path_tmp);
            fm->path_tmp = g_strdup (fm->path);
        }

    } else {
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
               "Failed to rename %s to %s", file_multi_get_name (fm), name);
    }

    return status;
}

/**
 * Returns the name of the file.
 *
 * @param fm Pointer to struct file_multi to return name for.
 * @return Name of struct file_multi.
 */
const gchar*
file_multi_get_name (struct file_multi *fm)
{
    g_assert (fm);

    if (! fm->name) {
        fm->name = file_multi_create_name (fm->path, fm->method);
    }

    return fm->name;
}

/**
 * Returns the ext of the file.
 *
 * @param fm Pointer to struct file_multi to return ext for.
 * @return Extension of struct file_multi.
 */
const gchar*
file_multi_get_ext (struct file_multi *fm)
{
    g_assert (fm);

    if (! fm->ext) {
        fm->ext = file_multi_create_ext (fm->path);
    }

    return fm->ext ? fm->ext : "";
}

/**
 * Returns the uri of the file.
 *
 * @param fm Pointer to struct file_multi to return uri for.
 * @return URI of struct file_multi.
 */
const gchar*
file_multi_get_uri (struct file_multi *fm)
{
    g_assert (fm);

    if (! fm->uri) {
        fm->uri = file_multi_create_uri (fm->path, fm->method);
    }

    return fm->uri;
}

/**
 * Returns the directory where the file is stored.
 *
 * @param fm Pointer to struct file_multi to return directory for.
 * @return Directory where file resides.
 */
const gchar*
file_multi_get_dir (struct file_multi *fm)
{
    g_assert (fm);

    if (! fm->dir) {
        fm->dir = file_multi_create_dir (fm->path);
    }

    return fm->dir;
}

/**
 * Returns the path used to open file.
 *
 * @param fm Pointer to struct file_multi to return name for.
 * @return Path used to open file.
 */
const gchar*
file_multi_get_path (struct file_multi *fm)
{
    g_assert (fm);

    if (fm->path_tmp) {
        return fm->path_tmp;
    } else {
        return fm->path;
    }
}

/**
 * Returns the mtime of the file.
 *
 * @param fm Pointer to struct file_multi to get size for.
 * @return Size in bytes of file.
 */
off_t
file_multi_get_size (struct file_multi *fm)
{
    struct stat buf;

    g_assert (fm);

    /* size not already set, try get to fetch it */
    if (fm->size == -1) {
        if (g_stat (file_multi_get_path (fm), &buf)) {
            fm->size = buf.st_size;
        }
    }

    return fm->size;
}

/**
 * Returns the mtime of the file.
 *
 * @param fm Pointer to struct file_multi to get size for.
 * @return Time file was last modified in unix time.
 */
time_t
file_multi_get_mtime (struct file_multi *fm)
{
    struct stat buf;

    g_assert (fm);

    /* mtime not already set, try get to fetch it */
    if (fm->mtime == -1) {
        if (! g_stat (file_multi_get_path (fm), &buf)) {
            fm->mtime = buf.st_mtime;
        }
    }

    return fm->mtime;
}

/**
 * Fetch file if needed.
 *
 * @param fm Pointer to struct file_multi to fetch file for.
 * @param stop Pointer to stop flag.
 * @return TRUE on success, else FALSE.
 */
gboolean
file_multi_fetch (struct file_multi *fm, gboolean *stop)
{
    gboolean status;

    g_assert (fm);

    if (! fm->need_fetch || fm->path_tmp) {
        return TRUE;
    }

    /* Get path to temporary file */
    fm->path_tmp = file_multi_create_tmpname ();
    if (! fm->path_tmp) {
        g_warning ("failed to get temporary file for fetching");

        return FALSE;
    }

    /* Fetch based on method */
    switch (fm->method) {
    case FILE_MULTI_METHOD_STDIN:
        status = file_multi_fetch_stdin (fm, stop);
        break;
    case FILE_MULTI_METHOD_HTTP:
        status = file_multi_fetch_http (fm, stop);
        break;
    case FILE_MULTI_METHOD_FTP:
        status = file_multi_fetch_ftp (fm, stop);
        break;
    default:
        /* Unknown method */
        status = FALSE;
    }

    return status;
}

/**
 * Returns wheter or not the file needs fetching.
 *
 * @param fm Pointer to struct file_multi to return fetch flag for.
 * @return TRUE if file needs fetching, else false.
 */
gboolean
file_multi_need_fetch (struct file_multi *fm)
{
    g_assert (fm);

    return fm->need_fetch;
}

/**
 * Figures the method needed to fetch the file based on the path.
 *
 * @param path Path to identify method for.
 */
guint
file_multi_get_method (const gchar *path)
{
    guint method;

    g_assert (path);

    if ((path[0] == '-') && (path[1] == '\0')) {
        method = FILE_MULTI_METHOD_STDIN;
    } else if ((util_stripos (path, "http://") == path)
               || (util_stripos (path, "https://") == path)) {
        method = FILE_MULTI_METHOD_HTTP;
    } else if (util_stripos (path, "ftp://")) {
        method = FILE_MULTI_METHOD_FTP;
    } else {
        method = FILE_MULTI_METHOD_PLAIN;
    }

    return method;
}

/**
 * Creates basename from path based on type.
 *
 * @param path Path to file.
 * @param method Method of file.
 * @return pointer to name, needs freeing.
 */
gchar*
file_multi_create_name (const gchar *path, guint method)
{
    gchar *name;

    if (method == FILE_MULTI_METHOD_STDIN) {
        name = g_strdup ("stdin");
    } else {
        name = g_path_get_basename (path);
    }

    return name;
}

/**
 * Creates extension from path.
 *
 * @param path Path to file.
 * @return pointer to extension, needs freeing.
 */
gchar*
file_multi_create_ext (const gchar *path)
{
    gchar *ext;

    ext = strrchr (path, '.');
    if (ext) {
        ext = g_strdup (ext + 1);
    }

    return ext;
}

/**
 * Creates dir from path.
 *
 * @param path Path to file.
 * @return Directory of path.
 */
gchar*
file_multi_create_dir (const gchar *path)
{
    gchar *dir;

    dir = g_strdup (path);
    dir = dirname (dir);

    return dir;
}

/**
 * Creates uri from path based on type.
 *
 * @param path Path to file.
 * @param method Method of file.
 * @return pointer to uri, needs freeing.
 */
gchar*
file_multi_create_uri (const gchar *path, guint method)
{
    gchar *uri;

    if (method == FILE_MULTI_METHOD_STDIN) {
        /* Stdin can not be URIified */
        uri = g_strdup ("stdin");

    } else if (method == FILE_MULTI_METHOD_PLAIN) {
        /* Standard file, make sure ~ is expanded and path is absolute */
        if (path[0] == '~') {
            uri = g_strjoin ("/", "file:/", g_get_home_dir (), path + 1, NULL);

        } else if (path[0] == '/') {
            /* Absolute path, append file:// */
            uri = g_strjoin (NULL, "file://", path, NULL);
        } else {
            /* Relative path, append file:// + current directory */
            uri = g_strjoin ("/", "file:/", g_get_current_dir (), path, NULL);
        }

    } else {
        /* Already an URI for the other methods */
        uri = g_strdup (path);
    }

    return uri;
}

/**
 * Creates name to temporary file.
 *
 * @return pointer to temporary name, needs freeing.
 */
gchar*
file_multi_create_tmpname (void)
{
    gint fd;
    gchar *path = g_strdup("/tmp/geh_XXXXXX");

    /* Create temporary file */
    fd = g_mkstemp (path);
    if (fd == -1) {
        g_free (path);
        return NULL;
    }

    return path;
}

/**
 * Fetch stdin, read until end of file and put into tmp file.
 *
 * @param fm Pointer to struct file_multi.
 * @param stop Pointer to stop flag.
 * @return Always returns TRUE.
 */
gboolean
file_multi_fetch_stdin (struct file_multi *fm, gboolean *stop)
{
    gchar buf[BUF_STDIN];
    size_t buf_read;
    FILE *out;

    /* Create output file */
    out = g_fopen (fm->path_tmp, "wb");
    if (! out) {
        g_warning ("Unable to write to temporary file %s", fm->path_tmp);

        return FALSE;
    }

    /* Read all of stdin and write to out */
    while ((buf_read = read (0 /* stdin */, buf, BUF_STDIN)) > 0) {
        fwrite (buf, 1, buf_read, out);
    }

    /* Close file */
    fclose (out);

    return TRUE;
}

/**
 * Fetch http url pointer to by file_multi to file_tmp.
 *
 * @param fm Pointer to struct file_multi.
 * @param stop Pointer to stop flag.
 * @return TRUE on success, else FALSE.
 */
gboolean
file_multi_fetch_http (struct file_multi *fm, gboolean *stop)
{
    return file_multi_fetch_wget (fm, stop);
}

/**
 * Fetch ftp url pointer to by file_multi to file_tmp.
 *
 * @param fm Pointer to struct file_multi.
 * @param stop Pointer to stop flag.
 * @return TRUE on success, else FALSE.
 */
gboolean
file_multi_fetch_ftp (struct file_multi *fm, gboolean *stop)
{
    return file_multi_fetch_wget (fm, stop);
}

/**
 * Fetch path pointed to by file_multi to file_tmp.
 *
 * @param fm Pointer to struct file_multi.
 * @param stop Pointer to stop flag.
 * @return TRUE on success, else FALSE.
 * @todo Implement configurable user agent string.
 */
gboolean
file_multi_fetch_wget (struct file_multi *fm, gboolean *stop)
{
    gchar *argv[5];
    gint status = 0, child_done = 0, child_status = 1;
    GPid child_pid;

    GError *err = NULL;

    /* Do nothing if stop flag */
    if (*stop) {
        return FALSE;
    }

    /* Build command */
    argv[0] = "wget";
    argv[1] = "-O";
    argv[2] = fm->path_tmp;
    argv[3] = fm->path;
    argv[4] = NULL;

    if (g_spawn_async (NULL /* working_directory */, argv,
                       NULL, WGET_SPAWN_FLAGS,
                       NULL /* child_setup */, NULL /* user_data */,
                       &child_pid, &err)) {
        /* Wait for child to finish */
        while (! *stop && ! child_done && status != -1) {
            status = waitpid (child_pid, &child_status, WNOHANG);
            if (status > 0) {
                child_done = 1;
            } else {
                g_usleep (WGET_CHECK_INTERVAL);
            }
        }

        /* Kill child and wait for it to exit */
        if (! child_done) {
            kill (child_pid, WGET_KILL_SIGNAL);
            g_usleep (WGET_CHECK_INTERVAL);
            if (waitpid (child_pid, &child_status, WNOHANG) < 1) {
                /* Failed to stop, really kill */
                g_usleep (WGET_KILL_WAIT);
                kill (child_pid, WGET_DIE_SIGNAL);
                waitpid (child_pid, &child_status, 0);
            }
        }

        if (child_status == 0) {
            /* Succeeded to fetch file, set need_fetch flag */
            fm->need_fetch = FALSE;      
        } else {
            /* Failed to fetch file, clear the tmp flag */
            if (child_done) {
                g_fprintf (stderr, "Unable to fetch %s\n", fm->path);
            }
            file_multi_close_tmp (fm);
        }

        /* Cleanup */
        g_spawn_close_pid (child_pid);

    } else if (err) {
        g_fprintf (stderr, "%s\n", err->message);
        g_error_free (err);
    }

    return ! fm->need_fetch;
}
