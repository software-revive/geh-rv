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
 * Image handling, represents the original image and the current
 * version (rotation, scaling etc)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "image.h"
#include "orientation.h"

static void image_update (struct image *im);

/**
 * Creates new struct image populated with image from file.
 *
 * @param path Path to image.
 * @return struct image on success, else NULL.
 */
struct image*
image_open (const gchar *path)
{
    struct image *im;
    GError *err = NULL;

    im = g_malloc (sizeof (struct image));

    /* Load original file */
    im->pix_orig = gdk_pixbuf_new_from_file (path, &err);
    if (err || !im->pix_orig) {
        /* Print error message */
        if (err) {
            g_fprintf (stderr, "%s\n", err->message);
            g_error_free (err);
        }

        /* Free image resources */
        g_free (im);
        return NULL;
    }
    
    im->width_orig = im->width_r_orig = gdk_pixbuf_get_width (im->pix_orig);
    im->height_orig = im->height_r_orig = gdk_pixbuf_get_height (im->pix_orig);

    /* Update for orientation */
    const gchar *orientation = gdk_pixbuf_get_option(im->pix_orig, "orientation");
    if (orientation != NULL) {
        orientation_transform (&im->pix_orig, &im->width_orig, &im->height_orig,
                               orientation);
    }

    /* Setup current representation */
    im->pix_curr = gdk_pixbuf_copy (im->pix_orig);
    im->width_curr = im->width_orig;
    im->height_curr = im->height_orig;
    im->zoom = 100;
    im->rotation = 0;
    
    return im;
}

/**
 * Frees resources used by struct image.
 *
 * @param im Pointer to struct image
 */
void
image_close (struct image *im)
{
    g_assert (im);

    g_object_unref (im->pix_orig);
    g_object_unref (im->pix_curr);

    g_free (im);
}

/**
 * Returns the current representation of the image.
 *
 * @param im Pointer to struct image.
 * @return Pointer to GdkPixbuf.
 */
GdkPixbuf*
image_get_curr (struct image *im)
{
    g_assert (im);

    return im->pix_curr;
}

/**
 * Zoom relative to the current zoom.
 *
 * @param im Pointer to struct image to zoom.
 * @param zoom Percent to in/decrease zoom with.
 * @return Resulting zoom percent.
 */
guint
image_zoom (struct image *im, gint zoom)
{
    g_assert (im);

    /* Zoom */
    image_zoom_set (im, im->zoom + zoom);

    return zoom;
}

/**
 * Zoom absolute.
 *
 * @param im Pointer to struct image to zoom.
 * @param zoom Zoom percentage to set.
 */
void
image_zoom_set (struct image *im, guint zoom)
{
    g_assert (im);

    /* Sanity check */
    if (zoom < 10) {
        zoom = 10;
    }

    if (im->zoom != zoom) {
        im->zoom = zoom;
        image_update (im);
    }
}

/**
 * Sets the zoom factor to match the available width and height.
 *
 * @param width Available width.
 * @param height Available height.
 */
void
image_zoom_fit (struct image *im, guint width, guint height)
{
    guint zoom;
    gfloat ratio, view_ratio;

    g_assert (im);

    /* No need to scale down if image fits in view. */
    if ((im->width_r_orig < width) || (im->height_r_orig < height)) {
        image_zoom_set (im, 100);

    } else {
        /* Get ratio */
        ratio = (gfloat) im->width_r_orig / (gfloat) im->height_r_orig;
        view_ratio = (gfloat) width / (gfloat) height;

        if (view_ratio > ratio) {
            zoom = (gint) ((gfloat) height / (gfloat) im->height_r_orig * 100);
        } else {
            zoom = (gint) ((gfloat) width / (gfloat) im->width_r_orig * 100);
        }

        image_zoom_set (im, zoom);
    }
}

/**
 * Rotates relativ to current rotation.
 *
 * @param im Pointer to struct image to rotate.
 * @param rotation Degrees to rotate.
 * @return Resulting rotation.
 */
guint
image_rotate (struct image *im, gint rotation)
{
    g_assert (im);

    rotation = rotation + im->rotation;

    if (rotation > 0) {
        /* Rotation left */
        while (rotation > 360) {
            rotation -= 360;
        }
    } else if (rotation < 0) {
        /* Rotation right */
        while (rotation < 0) {
            rotation += 360;
        }
    }

    image_rotate_set (im, rotation);

    return rotation;
}

/**
 * Rotate absolute.
 *
 * @param im Pointer to struct image to rotate.
 * @param rotation Degrees to rotate.
 */
void
image_rotate_set (struct image *im, guint rotation)
{
    g_assert (im);

    /* Currently gdk_pixbuf only rotates in multiples of 90 */
    rotation -= rotation % 90;

    /* Sanity check rotation */
    if ((rotation < 0) || (rotation >= 360)) {
        rotation = 0;
    }

    im->rotation = rotation;
    /* Set rotated original size */
    if ((rotation == 90) || (rotation == 270)) {
        im->width_r_orig = im->height_orig;
        im->height_r_orig = im->width_orig;
    } else {
        im->width_r_orig = im->width_orig;
        im->height_r_orig = im->height_orig;
    }

    image_update (im);
}

/**
 * Updates current image by scaling and rotating.
 *
 * @param im Pointer to struct image to update.
 */
void
image_update (struct image *im)
{
    GdkPixbuf *pix_tmp;
    guint width, height;

    /* Clean old resources */
    g_object_unref (im->pix_curr);

    if (!im->rotation && (im->zoom == 100)) {
        /* No modifications, use original */
        im->pix_curr = gdk_pixbuf_copy (im->pix_orig);

    } else {
        /* Rotate */
        if (im->rotation != 0) {
            im->pix_curr = gdk_pixbuf_rotate_simple (im->pix_orig, 
                                                     im->rotation);
        } else {
            im->pix_curr = im->pix_orig;
        }

        /* Zoom */
        if (im->zoom != 100) {
            width = gdk_pixbuf_get_width (im->pix_curr);
            height = gdk_pixbuf_get_height (im->pix_curr);
            im->width_curr =  width * (im->zoom * 0.01);
            im->height_curr = height * (im->zoom * 0.01);

            /* Scale */
            pix_tmp = im->pix_curr;
            im->pix_curr = gdk_pixbuf_scale_simple (im->pix_curr,
                                                    im->width_curr,
                                                    im->height_curr,
                                                    GDK_INTERP_BILINEAR);

            /* Clean resources */
            if (pix_tmp != im->pix_orig) {
                g_object_unref (pix_tmp);
            }
        }
    }

    /* Update size */
    im->width_curr = gdk_pixbuf_get_width (im->pix_curr);
    im->height_curr = gdk_pixbuf_get_height (im->pix_curr);
}
