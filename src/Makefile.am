
SUBDIRS = . $(SRCSUBDIRS)
DIST_SUBDIRS = cli wxgui

BUILT_SOURCES = $(srcdir)/swigluarun.h $(srcdir)/../swig/fityk_lua.cpp

lib_LTLIBRARIES = libfityk.la

libfityk_la_LDFLAGS = $(LIBRARY_VERSION_FLAG)
libfityk_la_LIBADD = -lxy -lz $(LUA_LIB)
libfityk_la_CPPFLAGS = $(LUA_INCLUDE)

$(srcdir)/swigluarun.h:
	$(SWIG) -c++ -lua -external-runtime $@

# this rule is also in swig/Makefile.am
$(srcdir)/../swig/fityk_lua.cpp: $(srcdir)/../swig/fityk.i  $(srcdir)/fityk.h
	$(SWIG) -c++ -lua -I$(srcdir) -o $@ $<

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
		 settings.h ui.h GAfit.h LMfit.h guess.h NMfit.h \
		 model.h fit.h voigt.h numfuncs.h

if LUA_ENABLED
libfityk_la_SOURCES += $(srcdir)/../swig/fityk_lua.cpp
endif

include_HEADERS = fityk.h

