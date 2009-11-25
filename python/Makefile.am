# $Id: Makefile.am 238 2007-01-21 01:44:04Z wojdyr $
EXTRA_DIST =

SWIG_SOURCES = fityk.i
BUILT_SOURCES = $(srcdir)/fityk_python.cpp $(srcdir)/fityk_lua.cpp
python_PYTHON = $(srcdir)/fityk.py
pyexec_LTLIBRARIES = _fityk.la
_fityk_la_SOURCES = $(srcdir)/fityk_python.cpp $(SWIG_SOURCES)
_fityk_la_CPPFLAGS = $(SWIG_PYTHON_CPPFLAGS) -I$(top_srcdir)/src
_fityk_la_LDFLAGS = @LIBRARY_VERSION_FLAG@ -module
_fityk_la_LIBADD = ../src/libfityk.la

$(srcdir)/fityk_python.cpp : $(SWIG_SOURCES) $(top_srcdir)/src/fityk.h
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src -o $@ $<
$(srcdir)/fityk.py : $(srcdir)/fityk_python.cpp

$(srcdir)/fityk_lua.cpp : $(SWIG_SOURCES) $(top_srcdir)/src/fityk.h
	$(SWIG) -c++ -lua -I$(top_srcdir)/src -o $@ $<

lua:
	$(CXX) -fPIC -I/usr/include/lua5.1 -I$(top_srcdir)/src -c fityk_lua.cpp
	$(CXX) -shared fityk_lua.o -L$(top_srcdir)/src/.libs -lfityk -o fityk.so
