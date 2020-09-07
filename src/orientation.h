/**
 * Image orientation handling.
 */

#ifndef _ORIENTATION_H_
#define _ORIENTATION_H_

#include <gtk/gtk.h>

/**
 * Image orientation.
 */
enum orientation {
    TOP_LEFT_SIDE = 1, /* 0 */
    TOP_RIGHT_SIDE = 2, /* 0, Mirrored. */ 
    BOTTOM_RIGHT_SIDE = 3, /* 180 */
    BOTTOM_LEFT_SIDE = 4, /* 180, Mirrored. */
    LEFT_SIDE_TOP = 5, /* 90, Mirrored. */
    RIGHT_SIDE_TOP = 6, /* 270 */
    RIGHT_SIDE_BOTTOM = 7, /* 270, Mirrored. */
    LEFT_SIDE_BOTTOM = 8 /* 90 */
};


void orientation_transform (GdkPixbuf **pix_ret, guint *width, guint *height,
                            const gchar *orientation);

#endif /* _ORIENTATION_H_ */
