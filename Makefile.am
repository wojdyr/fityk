ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = fityk.iss fityk.url fityk.svg

SUBDIRS=
if DOC_DIR
SUBDIRS += doc
endif
SUBDIRS += src

icondir = $(datadir)/pixmaps
desktopdir = $(datadir)/applications
mimepkgdir = $(datadir)/mime/packages

dist_icon_DATA = fityk.png
dist_desktop_DATA = fityk.desktop
dist_mimepkg_DATA = fityk.xml


samplesdir = $(pkgdatadir)/samples
dist_samples_DATA = samples/nacl01.fit samples/nacl01.dat \
		    samples/SiC_Zn.fit samples/SiC_Zn.dat  \
		    samples/enso.fit samples/enso.dat \
		    samples/read-shockley.fit \
		    samples/test_syntax.fit \
		    samples/hello.cc \
		    samples/hello.py samples/hello.lua samples/hello.pl \
		    README

check_PROGRAMS = hello
hello_SOURCES = hello.cc
hello_CPPFLAGS = -I$(srcdir)/src
hello_LDADD = src/libfityk.la

