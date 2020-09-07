#include "orientation.h"

/**
 * Update *pix_ret for the specified orientation.
 */
void
orientation_transform (GdkPixbuf **pix_ret, guint *width, guint *height,
                       const gchar *orientation)
{
    gchar *endptr;
    gint64 orientation_ll = g_ascii_strtoll(orientation, &endptr, 10);
    if (*endptr != '\0') {
        /* Failed to parse the orientation, ignore. */
        return;
    }

    GdkPixbuf *pix = NULL;
    gint tmp;
    
    switch (orientation_ll) {
    case TOP_LEFT_SIDE:
    case TOP_RIGHT_SIDE:
        break;
    case BOTTOM_RIGHT_SIDE:
        pix = gdk_pixbuf_rotate_simple (*pix_ret, 180);
        break;
    case BOTTOM_LEFT_SIDE:
    case LEFT_SIDE_TOP:
        break;
    case RIGHT_SIDE_TOP:
        pix = gdk_pixbuf_rotate_simple (*pix_ret, 270);
        tmp = *width;
        *width = *height;
        *height = tmp;
        break;
    case RIGHT_SIDE_BOTTOM:
        break;
    case LEFT_SIDE_BOTTOM:
        pix = gdk_pixbuf_rotate_simple (*pix_ret, 90);
        tmp = *width;
        *width = *height;
        *height = tmp;        
        break;
    default:
        break;
    };

    if (pix != NULL) {
        g_object_unref (*pix_ret);
        *pix_ret = pix;
    }
    
}
