
SUBDIRS = .
if CLI_DIR
bin_PROGRAMS = cli/cfityk
endif
if GUI_DIR
SUBDIRS += wxgui
endif

EXTRA_DIST = swig/fityk_perl.cpp swig/Fityk.pm swig/fityk.i
BUILT_SOURCES = swig/fityk_lua.cpp swig/luarun.h

lib_LTLIBRARIES = libfityk.la

libfityk_la_LDFLAGS = $(LIBRARY_VERSION_FLAG)
libfityk_la_LIBADD = -lxy -lz $(LUA_LIB)
libfityk_la_CPPFLAGS = $(LUA_INCLUDE)


libfityk_la_SOURCES = logic.cpp view.cpp lexer.cpp eparser.cpp cparser.cpp \
		 runner.cpp info.cpp common.cpp data.cpp var.cpp mgr.cpp \
		 tplate.cpp func.cpp udf.cpp bfunc.cpp ast.cpp \
		 vm.cpp transform.cpp \
		 settings.cpp ui.cpp GAfit.cpp LMfit.cpp guess.cpp NMfit.cpp \
		 model.cpp fit.cpp voigt.cpp numfuncs.cpp fityk.cpp \
		 \
                 logic.h view.h lexer.h eparser.h cparser.h \
		 runner.h info.h common.h data.h var.h mgr.h \
		 tplate.h func.h udf.h bfunc.h ast.h \
		 vm.h transform.h \
		 settings.h ui.h ui_api.h GAfit.h LMfit.h guess.h NMfit.h \
		 model.h fit.h voigt.h numfuncs.h \
		 swig/fityk_lua.cpp swig/luarun.h \
		 MPfit.cpp MPfit.h cmpfit/mpfit.c cmpfit/mpfit.h

if NLOPT_ENABLED
libfityk_la_SOURCES += NLfit.cpp NLfit.h
endif

include_HEADERS = fityk.h

# ---  directory cli/ ---
cli_cfityk_SOURCES = cli/gnuplot.cpp cli/main.cpp cli/gnuplot.h
cli_cfityk_LDADD = libfityk.la $(READLINE_LIBS)


# --- directory swig/ ---

python_PYTHON = $(srcdir)/swig/fityk.py

if PYTHON_ENABLED
pyexec_LTLIBRARIES = swig/_fityk.la
endif

swig__fityk_la_SOURCES = swig/fityk_python.cpp
swig__fityk_la_CPPFLAGS = $(PYTHON_CPPFLAGS) -I$(srcdir)
swig__fityk_la_LDFLAGS = $(LIBRARY_VERSION_FLAG) -module
swig__fityk_la_LIBADD = libfityk.la

$(srcdir)/swig/luarun.h:
	$(SWIG) -c++ -lua -external-runtime $@

$(srcdir)/swig/fityk_python.cpp: swig/fityk.i fityk.h
	$(SWIG) -c++ -python -I$(srcdir) -o $@ $<
$(srcdir)/swig/fityk.py: $(srcdir)/swig/fityk_python.cpp

$(srcdir)/swig/fityk_lua.cpp: swig/fityk.i fityk.h
	$(SWIG) -c++ -lua -I$(srcdir) -o $@ $<

$(srcdir)/swig/fityk_perl.cpp: swig/fityk.i fityk.h
	$(SWIG) -c++ -perl -I$(srcdir) -o $@ $<
$(srcdir)/swig/Fityk.pm: $(srcdir)/swig/fityk_perl.cpp

perl: swig/fityk_perl.o
	$(CXX) -shared $< -L./.libs -lfityk -o Fityk.so

# requires perl-ExtUtils-Embed
swig/fityk_perl.o: swig/fityk_perl.cpp
	$(CXX) -fPIC `perl -MExtUtils::Embed -e perl_inc` -I$(srcdir) -o $@ -c $<

