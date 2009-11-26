# $Id$

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = fityk.iss fityk.url fityk.svg \
	     m4/readline.m4 \
	     3rdparty/README 3rdparty/boost \
	     macosx/README.txt macosx/Makefile macosx/Info.plist.in \
	     macosx/fityk.icns
AUTOMAKE_OPTIONS = dist-bzip2

SUBDIRS=
if XYLIB_DIR
SUBDIRS += xylib
endif
if DOC_DIR
SUBDIRS += doc
endif
# src must be built after xylib
SUBDIRS += src
if SAMPLES_DIR
SUBDIRS += samples
endif
if SWIG_DIR
SUBDIRS += swig
endif

icondir = $(datadir)/pixmaps
desktopdir = $(datadir)/applications
mimepkgdir = $(datadir)/mime/packages

dist_icon_DATA = fityk.png
dist_desktop_DATA = fityk.desktop
dist_mimepkg_DATA = fityk.xml

