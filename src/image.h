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

#ifndef _IMAGE_H_
#define _IMAGE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/**
 * Main structure reprsenting modifiable image.
 */
struct image {
    GdkPixbuf *pix_orig; /**< Original image */
    GdkPixbuf *pix_curr; /**< Current image */

    guint width_orig; /**< Original width */
    guint height_orig; /**< Original height */
    guint width_r_orig; /**< Rotated width of the original size */
    guint height_r_orig; /**< Rotated height of the original size */
    guint width_curr; /**< Current width */
    guint height_curr; /**< Current height */

    guint zoom; /**< Zoom percentage. */
    guint rotation; /**< Rotation degrees. */
};

struct image *image_open (const gchar *path);
void image_close (struct image *im);

GdkPixbuf *image_get_curr (struct image *im);

guint image_zoom (struct image *im, gint zoom);
void image_zoom_set (struct image *im, guint zoom);
void image_zoom_fit (struct image *im, guint width, guint height);

guint image_rotate (struct image *im, gint rotation);
void image_rotate_set (struct image *im, guint rotation);

#endif /* _IMAGE_H_ */
