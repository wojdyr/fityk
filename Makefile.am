
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = fityk.iss fityk.url fityk.svg \
	     m4/readline.m4 \
	     macosx/README.txt macosx/Makefile macosx/Info.plist.in \
	     macosx/fityk.icns
AUTOMAKE_OPTIONS = dist-bzip2

SUBDIRS=
if DOC_DIR
SUBDIRS += doc
endif
SUBDIRS += src
SUBDIRS += samples
if SWIG_DIR
SUBDIRS += swig
endif

icondir = $(datadir)/pixmaps
desktopdir = $(datadir)/applications
mimepkgdir = $(datadir)/mime/packages

dist_icon_DATA = fityk.png
dist_desktop_DATA = fityk.desktop
dist_mimepkg_DATA = fityk.xml

