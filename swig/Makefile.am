# $Id: Makefile.am 238 2007-01-21 01:44:04Z wojdyr $
EXTRA_DIST =

SWIG_SOURCES = fityk.i
BUILT_SOURCES = $(srcdir)/fityk_python.cpp $(srcdir)/fityk_lua.cpp \
		$(srcdir)/fityk_perl.cpp $(srcdir)/Fityk.pm
python_PYTHON = $(srcdir)/fityk.py

if PYTHON_ENABLED
pyexec_LTLIBRARIES = _fityk.la
endif

if LUA_ENABLED
noinst_LTLIBRARIES = fityk.la
endif

_fityk_la_SOURCES = $(srcdir)/fityk_python.cpp $(SWIG_SOURCES)
_fityk_la_CPPFLAGS = $(PYTHON_CPPFLAGS) -I$(top_srcdir)/src
_fityk_la_LDFLAGS = $(LIBRARY_VERSION_FLAG) -module
_fityk_la_LIBADD = ../src/libfityk.la

fityk_la_SOURCES = $(srcdir)/fityk_lua.cpp $(SWIG_SOURCES)
fityk_la_CPPFLAGS = $(LUA_INCLUDE) -I$(top_srcdir)/src
fityk_la_LDFLAGS = $(LIBRARY_VERSION_FLAG) -module
fityk_la_LIBADD = ../src/libfityk.la

$(srcdir)/fityk_python.cpp: $(SWIG_SOURCES) $(top_srcdir)/src/fityk.h
	$(SWIG) -c++ -python -I$(top_srcdir)/src -o $@ $<
$(srcdir)/fityk.py: $(srcdir)/fityk_python.cpp

$(srcdir)/fityk_lua.cpp: $(SWIG_SOURCES) $(top_srcdir)/src/fityk.h
	$(SWIG) -c++ -lua -I$(top_srcdir)/src -o $@ $<

$(srcdir)/fityk_perl.cpp: $(SWIG_SOURCES) $(top_srcdir)/src/fityk.h
	$(SWIG) -c++ -perl -I$(top_srcdir)/src -o $@ $<
$(srcdir)/Fityk.pm: $(srcdir)/fityk_perl.cpp


LIBFITYK_LINK=-L$(top_srcdir)/src/.libs -lfityk

#lua: $(srcdir)/fityk_lua.cpp
#	$(CXX) -fPIC -I/usr/include/lua5.1 -I$(top_srcdir)/src -c fityk_lua.cpp
#	$(CXX) -shared fityk_lua.o $(LIBFITYK_LINK) -o fityk.so

perl: $(srcdir)/fityk_perl.cpp
	$(CXX) -fPIC `perl -MExtUtils::Embed -e perl_inc` -I$(top_srcdir)/src -c fityk_perl.cpp
	$(CXX) -shared fityk_perl.o $(LIBFITYK_LINK) -o Fityk.so

