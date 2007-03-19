# $Id: Makefile.am 238 2007-01-21 01:44:04Z wojdyr $
EXTRA_DIST = 

BUILT_SOURCES = $(srcdir)/fityk_wrap.cpp
SWIG_SOURCES = fityk.i
python_PYTHON = fityk.py
pyexec_LTLIBRARIES = _fityk.la
_fityk_la_SOURCES = $(srcdir)/fityk_wrap.cpp $(SWIG_SOURCES)
_fityk_la_CPPFLAGS = $(SWIG_PYTHON_CPPFLAGS) -I$(top_srcdir)/src
_fityk_la_LDFLAGS = -module
_fityk_la_LIBADD = ../src/libfityk.la

$(srcdir)/fityk_wrap.cpp : $(SWIG_SOURCES)
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src -o $@ $<

