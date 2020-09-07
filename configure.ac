dnl
dnl Copyright (c) 2006 Claes Nästén <me@pekdon.net>
dnl
dnl Permission is hereby granted, free of charge, to any person
dnl obtaining a copy of this software and associated documentation
dnl files (the "Software"), to deal in the Software without
dnl restriction, including without limitation the rights to use, copy,
dnl modify, merge, publish, distribute, sublicense, and/or sell copies
dnl of the Software, and to permit persons to whom the Software is
dnl furnished to do so, subject to the following conditions:
dnl
dnl The above copyright notice and this permission notice shall be
dnl included in all copies or substantial portions of the Software.
dnl
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
dnl EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
dnl MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
dnl NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
dnl BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
dnl ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
dnl CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
dnl SOFTWARE.
dnl

dnl init autoconf / automake
AC_INIT(geh, 0.3.0)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

dnl search for programs
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

dnl search for headers
AC_STDC_HEADERS
AC_HAVE_HEADERS(sys/types.h sys/stat.h unistd.h locale.h)

dnl check required libraries
AC_PATH_X
AC_PATH_XTRA

CFLAGS="$CFLAGS $X_FLAGS"
LIBS="$LIBS $X_LIBS"
LDFLAGS="$LDFLAGS $LIBS $X_PRE_LIBS"

AC_CHECK_LIB(X11, XOpenDisplay, LIBS="$LIBS -lX11", AC_MSG_ERROR([Could not find XOpenDisplay in -lX11.]))

dnl check required packages
AC_MSG_CHECKING([whether to build with Gtk2 instead of Gtk3])
AC_ARG_ENABLE(gtk2,
              AC_HELP_STRING([--enable-gtk2], [enable Gtk2 instead of Gkt3 [default=no]]),
              , [enable_gtk2=no])
if test "x$enable_gtk2" = "xyes"; then
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

PKG_CHECK_MODULES([gtk3], [gtk+-3.0 >= 3.0.0], HAVE_GTK3=yes, HAVE_GTK3=no)
if test x${enable_gtk2}x${HAVE_GTK3} = xnoxyes; then
    AC_DEFINE(HAVE_GTK3, [1], [define to 1 if you have gtk+-3.0])
else
    PKG_CHECK_MODULES([gtk2], [gtk+-2.0 >= 2.8.0], HAVE_GTK2=yes, HAVE_GTK2=no)
    if test "x$HAVE_GTK2" = "xyes"; then
       AC_DEFINE(HAVE_GTK2, [1], [define to 1 if you have gtk+-2.0])
    else
       AC_MSG_ERROR([Could not find gtk+-3.09 >= 3.0.0 or gtk+-2.0 >= 2.8.0, required dependency.])
    fi
fi

AM_CONDITIONAL(GTK3, test x${enable_gtk2}x${HAVE_GTK3} = xnoxyes)
AM_CONDITIONAL(GTK2, test x$HAVE_GTK2 = xyes)

dnl output makefiles
AC_OUTPUT(Makefile doc/Makefile src/Makefile)
