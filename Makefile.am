#
# Copyright (c) 2006 Claes Nästén <me@pekdon.net>
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

# $Id: Makefile.am 53 2006-08-02 16:20:49Z me@pekdon.net $

# Without this GNU make is required
AUTOMAKE_OPTIONS = foreign

NULL =

SUBDIRS = src

doc_DATA = \
	AUTHORS \
	ChangeLog.pre-0.4.0 \
	LICENSE \
	NEWS \
	README.md \
	$(NULL)

EXTRA_DIST = \
	$(doc_DATA) \
	CMakeLists.txt \
	$(NULL)


distclean-local:
	rm -f *\~ .\#*
