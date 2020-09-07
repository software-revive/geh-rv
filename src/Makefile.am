#
# Copyright © 2006-2009 Claes Nästén <me@pekdon.net>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# Without this GNU make is required
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = geh
geh_SOURCES = dir.c dir.h \
	file_fetch.c file_fetch.h \
	file_fetch_img.c file_fetch_img.h \
	file_multi.c file_multi.h \
	file_queue.c file_queue.h \
	geh.h \
	image.c image.h \
	md5.c md5.h \
	orientation.c orientation.h \
	thumb.c thumb.h \
	ui_window.c ui_window.h \
	util.c util.h \
	main.c
geh_CFLAGS = -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE

if GTK3
geh_LDADD = @gtk3_LIBS@
AM_CPPFLAGS = @gtk3_CFLAGS@ -DHAVE_GTK3
else
geh_LDADD = @gtk2_LIBS@
AM_CPPFLAGS = @gtk2_CFLAGS@ -DHAVE_GTK2
endif

distclean-local:
	rm -f *\~ .\#*
