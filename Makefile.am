# $Id$

ACLOCAL_AMFLAGS = -I config
EXTRA_DIST = fityk.iss fityk.url fityk.svg \
	     config/readline.m4 \
	     3rdparty/README 3rdparty/boost \
	     macosx/README.txt macosx/Makefile macosx/Info.plist.in \
	     macosx/fityk.icns
AUTOMAKE_OPTIONS = dist-bzip2

SUBDIRS = src
if XYLIB_DIR
SUBDIRS += xylib
endif
if DOC_DIR
SUBDIRS += doc
endif
if SAMPLES_DIR
SUBDIRS += samples
endif
if SWIG_DIR
SUBDIRS += python
endif

icondir = $(datadir)/pixmaps
desktopdir = $(datadir)/applications
mimepkgdir = $(datadir)/mime/packages

dist_icon_DATA = fityk.png
dist_desktop_DATA = fityk.desktop
dist_mimepkg_DATA = fityk.xml

