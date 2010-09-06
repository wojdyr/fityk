# $Id$

samplesdir = $(pkgdatadir)/samples
dist_samples_DATA = \
		nacl01.fit  nacl01.dat \
		SiC_Zn.fit SiC_Zn.dat  \
		test_syntax.fit \
		test_lib.cc test_lib.py test_lib.lua test_lib.pl \
		README

# Compile test_lib.cc, useful only for testing.
#if COMPILE_LIB_TEST
noinst_PROGRAMS = test_lib
test_lib_SOURCES = test_lib.cc
test_lib_CPPFLAGS = -I$(top_srcdir)/src
test_lib_LDADD = ../src/libfityk.la
#endif

