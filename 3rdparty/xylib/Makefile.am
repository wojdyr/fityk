# $Id: Makefile.am 354 2007-11-19 12:38:52Z wojdyr $
SUBDIRS = xylib

noinst_PROGRAMS = xyconv

xyconv_SOURCES = xyconv.cpp

AM_CPPFLAGS = @CPPFLAGS_3RDPARTY@ 
LDADD = xylib/libxy.la -lm

