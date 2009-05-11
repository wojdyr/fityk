# $Id$

ACLOCAL_AMFLAGS = -I config
EXTRA_DIST = fityk.spec fityk.iss fityk.url fityk.svg macosx/
AUTOMAKE_OPTIONS = dist-bzip2
SUBDIRS = @DOC@ 3rdparty src samples config @PYBINDINGDIR@

icondir = $(datadir)/pixmaps
desktopdir = $(datadir)/applications
mimepkgdir = $(datadir)/mime/packages

dist_icon_DATA = fityk.png
dist_desktop_DATA = fityk.desktop
dist_mimepkg_DATA = fityk.xml

