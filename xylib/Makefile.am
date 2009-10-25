# $Id: Makefile.am 354 2007-11-19 12:38:52Z wojdyr $
SUBDIRS = xylib

EXTRA_DIST = web.css sample-urls

bin_PROGRAMS = xyconv

dist_man_MANS = xyconv.1

xyconv_SOURCES = xyconv.cpp

LDADD = xylib/libxy.la -lm

index.html: README
	rst2html --stylesheet-path=web.css README index.html
