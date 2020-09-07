/*
 * Copyright (c) 2006 Claes Nästén <me@pekdon.net>
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
 * UI window handling code.
 */

#ifndef _UI_H_
#define _UI_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>

#include "file_multi.h"
#include "image.h"

#define UI_ICON_STORE_FILE 0
#define UI_ICON_STORE_NAME 1
#define UI_ICON_STORE_THUMB 2
#define UI_ICON_STORE_FIELDS 3

#define UI_WINDOW_MODE_FULL 0
#define UI_WINDOW_MODE_SLIDE 1
#define UI_WINDOW_MODE_THUMB 2

#define UI_THUMB_PADDING 8
#define UI_THUMB_CHARS 14
#define UI_SLIDE_PADDING 84

/**
 * Struct defining UI window.
 */
struct ui_window {
  GtkWindow *window; /**< Window */
  gboolean is_fullscreen; /**< Set to TRUE when window is fullscreen. */
  gboolean zoom_fit; /**< Set to TRUE to zoom image to fit. */

  GtkBox *vbox; /**< Vertical box for pane + progress */
  GtkPaned *pane; /**< Main pain separating thumbnail view from image */
  guint width_alloc_prev; /**< Previous width allocation for window. */
  guint height_alloc_prev; /**< Previous height allocation for window. */

  GtkImage *image; /**< Image */
  GtkScrolledWindow *image_window; /** Image Area */

  GtkIconView *icon_view; /**< Thumbnail View */
  GtkScrolledWindow *icon_view_window; /** Thumbnail Area */
  GtkListStore *icon_store; /**< Thumbnail Store */
  GtkTreeIter icon_iter; /**< Thumbnail Store Iterator */
  GtkTreeIter icon_iter_add; /**< Thumbnail Store Iterator for adding data */
  guint thumbnails; /**< Number of thumbnails */

  guint mode; /**< Current mode of window. */
  struct file_multi *file; /**< Active file. */
  struct image *image_data; /**< Image wrapper for scaling/rotating. */

  GtkProgressBar *progress; /**< Progress bar for loading. */
  gint progress_total; /**< Total number to load. */
  gint progress_curr; /**< Current completed items. */
  gdouble progress_step; /**< Fraction size to increment. */
};

extern struct ui_window *ui_window_new (void);
extern void ui_window_free (struct ui_window *ui);

extern void ui_window_show (struct ui_window *ui);
extern void ui_window_hide (struct ui_window *ui);

extern guint ui_window_get_mode (struct ui_window *ui);
extern void ui_window_set_mode (struct ui_window *ui, guint mode);

extern void ui_window_set_image (struct ui_window *ui, struct file_multi *file,
                                 gboolean zoom_fit, gboolean lock);

extern void ui_window_add_thumbnail (struct ui_window *ui,
                                     struct file_multi *file, GdkPixbuf *pix);
extern void ui_window_clear_thumbnails (struct ui_window *ui);

extern void ui_window_progress_show (struct ui_window *ui, gboolean lock);
extern void ui_window_progress_hide (struct ui_window *ui, gboolean lock);
extern void ui_window_progress_progress (struct ui_window *ui, gint count,
                                         gboolean lock);
extern guint ui_window_progress_get_total (struct ui_window *ui);
extern void ui_window_progress_set_total (struct ui_window *ui, guint total);
extern void ui_window_progress_add (gpointer data, gint count);

#endif /* _UI_H_ */
