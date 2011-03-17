
samplesdir = $(pkgdatadir)/samples
dist_samples_DATA = \
		nacl01.fit  nacl01.dat \
		SiC_Zn.fit SiC_Zn.dat  \
		enso.fit enso.dat \
		read-shockley.fit \
		test_syntax.fit \
		hello.cc hello.py hello.lua hello.pl \
		README

# Compile hello.cc, useful only for testing.
#if COMPILE_LIB_TEST
noinst_PROGRAMS = hello
hello_SOURCES = hello.cc
hello_CPPFLAGS = -I$(top_srcdir)/src
hello_LDADD = ../src/libfityk.la
#endif

