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
 * Thumbnail generation and caching routines.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#define THUMB_LOAD_CHUNK_SIZE 8192

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "file_multi.h"
#include "md5.h"
#include "thumb.h"

/**
 * Struct used to feed information back to function generating thumbnail.
 */
struct thumb_image_info {
    guint side; /**< Side size to generate */
    gint width; /**< Original image width */
    gint height; /**< Original image height */
};

static GdkPixbuf *thumb_load (const gchar *path,
                              struct thumb_image_info *info);

static GdkPixbuf *thumb_cache_load (struct file_multi *file);
static void thumb_cache_save (struct file_multi *file, GdkPixbuf *thumb,
                              struct thumb_image_info *info);
static gboolean thumb_cache_save_create_directory (void);

static gchar *thumb_cache_path (struct file_multi *file);
static gchar *thumb_cache_md5_uri (const gchar *uri);

static void thumb_callback_size_prepared (GdkPixbufLoader *loader,
                                          gint width, gint height,
                                          gpointer user_data);

/**
 * Gets thumbnail for file at size.
 *
 * @param file struct file_multi to create thumbnail for.
 * @param side Maximum side in pixels for thumbnail
 * @param cache TRUE means cache generated thumbnail on disk.
 * @return Pointer to GdkPixbuf with thumbnail version of image.
 */
GdkPixbuf*
thumb_get (struct file_multi *file, guint side, gboolean cache)
{
    GdkPixbuf *thumb = NULL;
    gboolean cache_ok = ((side == THUMB_DEFAULT_SIDE)
                         || (side == THUMB_LARGE_SIDE));
    struct thumb_image_info info = {0 /* Side */,
                                    0 /* Width */, 0 /* Height */};

    /* Try load cached version */
    if (cache_ok) {
        thumb = thumb_cache_load (file);
    }

    /* Generate thumbnail */
    if (! thumb) {
        info.side = side;

        thumb = thumb_load (file_multi_get_path (file), &info);
        if (thumb && cache && cache_ok) {
            thumb_cache_save (file, thumb, &info);
        }
    }

    return thumb;
}

/**
 * Loads file at size.
 *
 * @param path File to load.
 * @param side Size in pixels max size of thumbnail.
 * @return Pointer to GdkPixbuf or NULL if fails.
 */
GdkPixbuf*
thumb_load (const gchar *path, struct thumb_image_info *info)
{
    GdkPixbuf *thumb;
    GdkPixbufLoader *loader;
    GError *err = NULL;

    int fd;
    guchar buf[THUMB_LOAD_CHUNK_SIZE];
    size_t buf_read;

    /* Create pixbuf loader */
    loader = gdk_pixbuf_loader_new ();

    /* Set callback so the image can be loaded at prefered size with
       aspect preserved. */
    g_signal_connect (G_OBJECT (loader), "size-prepared",
                      G_CALLBACK (thumb_callback_size_prepared), info);

    /* Open file for reading */
    fd = open (path, O_RDONLY);
    if (fd == -1) {
        g_warning ("failed to open %s for reading", path);
        return NULL;
    }

    /* Read all of file and write to loader */
    while ((buf_read = read (fd, buf, THUMB_LOAD_CHUNK_SIZE)) > 0) {
        if (! gdk_pixbuf_loader_write (loader, buf, buf_read, &err)) {
            /* Clean resources */
            gdk_pixbuf_loader_close (loader, NULL);
            g_object_unref (loader);
            close (fd);

            if (err) {
                g_fprintf (stderr, "%s\n", err->message);
                g_error_free (err);
            }

            return NULL;
        }
    }

    /* Close file after reading */
    close (fd);

    /* Finalize loading of image */
    if (! gdk_pixbuf_loader_close (loader, &err)) {
        g_object_unref (loader);

        if (err) {
            g_fprintf (stderr, "%s\n", err->message);
            g_error_free (err);
        }

        return NULL;
    }
    
    /* Get thumbnail */
    thumb = gdk_pixbuf_loader_get_pixbuf (loader);
    g_object_ref (thumb);

    const gchar *orientation = gdk_pixbuf_get_option(thumb, "orientation");
    if (orientation != NULL) {
        guint width = gdk_pixbuf_get_width (thumb);
        guint height = gdk_pixbuf_get_height (thumb);
        orientation_transform (&thumb, &width, &height, orientation);
    }

    /* Clean resources */
    g_object_unref (loader);

    return thumb;
}

/**
 * Load thumbnail from cache.
 *
 * @param file Original file.
 * @return GdkPixbuf representation of cached image, if none NULL.
 */
GdkPixbuf*
thumb_cache_load (struct file_multi *file)
{
    time_t mtime;
    gchar *thumb_path;
    const gchar *mtime_str;
    GdkPixbuf *thumb = NULL;

    /* Get thumbnail file */
    thumb_path = thumb_cache_path (file);
    if (g_file_test (thumb_path, G_FILE_TEST_IS_REGULAR)) {
        thumb = gdk_pixbuf_new_from_file (thumb_path, NULL);

        /* Check mtime */
        mtime_str = gdk_pixbuf_get_option (thumb, "tEXt::Thumb::MTime");
        if (mtime_str) {
            mtime = strtol (mtime_str, NULL, 10);
            if (mtime != file_multi_get_mtime (file)) {
                /* mtime does not match, treat as invalid */
                g_object_unref (thumb);
                thumb = NULL;
            }
        } else {
            /* No mtime to check against, treat as invalid */
            g_object_unref (thumb);
            thumb = NULL;
        }
    }

    /* Cleanup */
    g_free (thumb_path);

    return thumb;
}

/**
 * Save thumbnail to cache.
 *
 * @param file Original file.
 * @param thumb Pointer GdkPixbuf thumbnail to save.
 * @param info Pointer to struct thumb_image_info.
 */
void
thumb_cache_save (struct file_multi *file, GdkPixbuf *thumb,
                  struct thumb_image_info *info)
{
    gchar *thumb_path;
    gchar *size, *mtime, *width, *height;

    /* Make sure directory for saving exists */
    if (! thumb_cache_save_create_directory ()) {
        return;
    }

    /* Get string representation for file info */
    size = g_strdup_printf ("%li", file_multi_get_size (file));
    mtime = g_strdup_printf ("%li", file_multi_get_mtime (file));
    width = g_strdup_printf ("%d", info->width);
    height = g_strdup_printf ("%d", info->height);

    /* Get thumbnail file */
    thumb_path = thumb_cache_path (file);

    if (!  gdk_pixbuf_save (thumb, thumb_path, "png", NULL,
                            "tEXt::Thumb::URI", file_multi_get_uri (file),
                            "tEXt::Thumb::Size", size,
                            "tEXt::Thumb::MTime", mtime,
                            "tEXt::Thumb::Image::Width", width,
                            "tEXt::Thumb::Image::Height", height,
                            NULL)) {
        g_warning ("failed to save thumbnail for %s",
                   file_multi_get_path (file));
    }

    /* Cleanup */
    g_free (size);
    g_free (mtime);
    g_free (width);
    g_free (height);
    g_free (thumb_path);
}

/**
 * Makes sure thumbnail directory exists.
 *
 * @return Returns TRUE if it exists (or has been created), else FALSE.
 */
gboolean
thumb_cache_save_create_directory (void)
{
    static gboolean tried = FALSE;
    static gboolean status = FALSE;

    gchar *path, *path_base;

    if (! tried ) {
        /* Set tried flag */
        tried = TRUE;

        /* Check base thumbnail dir */
        path_base = g_strjoin (NULL, g_get_home_dir (),
                               THUMB_CACHE_PATH_BASE, NULL);
        if (g_file_test (path_base, G_FILE_TEST_IS_DIR)
            || (g_mkdir (path_base, 0700) != -1)) {
            /* Check thumbnail dir */
            path =  g_strjoin (NULL, g_get_home_dir (),
                               THUMB_CACHE_PATH, NULL);

            if (g_file_test (path, G_FILE_TEST_IS_DIR)
                || (g_mkdir (path, 0700) != -1)) {
                status = TRUE;
            }

            g_free (path);
        }

        g_free (path_base);
    }

    return status;
}

/**
 * Builds path to thumbnail file for file.
 *
 * @param file File to get thumbnail file for.
 * @return Path to thumbnail file for file, needs freeing.
 */
gchar*
thumb_cache_path (struct file_multi *file)
{
    gchar *md5, *md5_path;

    /* Get md5 representation of path */
    md5 = thumb_cache_md5_uri (file_multi_get_uri (file));

    /* Build path to thumb file */
    md5_path = g_strjoin (NULL, g_get_home_dir (),
                          THUMB_CACHE_PATH, md5, ".png", NULL);


    /* Clean up */
    g_free (md5);

    return md5_path;
}

/**
 * Creates HEX string representation of md5 value for uri.
 *
 * @param uri URI to MD5ify
 * @return String representation of MD5.
 */
gchar*
thumb_cache_md5_uri (const gchar *uri)
{
    gint i;
    gchar *md5;

    md5_state_t pms;
    md5_byte_t digest[16];

    /* Generate md5 sum. */
    md5_init (&pms);
    md5_append (&pms, (guchar*) uri, strlen (uri));
    md5_finish (&pms, digest);

    /* Print md5 hex. */
    md5 = g_malloc (sizeof (char) * 33);

    for (i = 0; i < 16; i++) {
        g_sprintf (md5 + i * 2, "%02x", digest[i]);
    }

    /* Terminate string */
    md5[32] = '\0';
  
    return md5;
} 

/**
 * Callback used when loading images making sure they are of the correct
 * size.
 *
 * @param loader Loader used to signal.
 * @param width Width of image being loaded.
 * @param height Height of image being loaded.
 * @param user_data User supplied data (pointer to side size)
 */
void
thumb_callback_size_prepared (GdkPixbufLoader *loader,
                              gint width, gint height, gpointer user_data)
{
    /* Get side and calculate ratio */
    guint side = *((guint*) user_data);
    gfloat ratio;

    /* Nothing to do, image fits in thumbnail size */
    if ((width <= side) && (height <= side)) {
        return;
    }

    /* Calculate ratio and set loader size */
    ratio =  (gfloat) width / (gfloat) height;
    if (width > height) {
        gdk_pixbuf_loader_set_size (loader, side, side / ratio);
    } else {
        gdk_pixbuf_loader_set_size (loader, side * ratio, side);
    }
}
