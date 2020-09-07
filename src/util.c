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
 * Utility functions.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdarg.h>

#include "util.h"

/**
 * Lazy case insensitive strpos.
 *
 * @param haystack String to search in.
 * @param needle String to find.
 * @return Pointer to needle in haystack if found, else NULL.
 */
const gchar*
util_stripos (const gchar *haystack, const gchar *needle)
{
    gchar *haystack_up = g_ascii_strup (g_strdup (haystack), strlen (haystack));
    gchar *needle_up = g_ascii_strup (g_strdup (needle), strlen (needle));

    const gchar *pos;

    /* Find needle in haystack */ 
    pos = strstr (haystack_up, needle_up);
    if (pos) {
        /* Get position in original stack */
        pos = haystack + ((pos - haystack_up) / sizeof (pos[0]));
    }

    /* Clean resources */
    g_free (haystack_up);
    g_free (needle_up);

    return pos;
}

/**
 * Checks if str is in list of arguments.
 *
 * @param casei If TRUE, case insensitive matching.
 * @param ... NULL terminated list of strings to check against.
 * @return TRUE if str is in list of strings, else FALSE.
 */
gboolean
util_str_in (const gchar *str, gboolean casei, ...)
{
    va_list ap;
    const gchar *str_cmp;

    va_start (ap, casei);
    while ((str_cmp = va_arg (ap, const gchar*)) != NULL) {
        if (casei) {
            if (g_ascii_strcasecmp (str, str_cmp) == 0) {
                return TRUE;
            }
        } else {
            if (strcmp (str, str_cmp) == 0) {
                return TRUE;
            }
        }
    }
    va_end (ap);
  
    return FALSE;
}
