# $Id$
SUBDIRS = . $(SRCSUBDIRS)
DIST_SUBDIRS = cli wxgui

lib_LTLIBRARIES = libfityk.la

libfityk_la_LDFLAGS = $(LIBRARY_VERSION_FLAG)
libfityk_la_LIBADD = -lxy

libfityk_la_SOURCES = logic.cpp view.cpp lexer.cpp eparser.cpp cparser.cpp \
		 runner.cpp info.cpp common.cpp data.cpp \
		 var.cpp mgr.cpp func.cpp udf.cpp bfunc.cpp calc.cpp ast.cpp \
		 settings.cpp ui.cpp GAfit.cpp LMfit.cpp guess.cpp NMfit.cpp \
		 model.cpp fit.cpp voigt.cpp numfuncs.cpp fityk.cpp \
		 \
                 common.h data.h lexer.h eparser.h cparser.h runner.h info.h \
		 settings.h GAfit.h LMfit.h guess.h NMfit.h logic.h view.h \
		 model.h fit.h ui.h voigt.h numfuncs.h var.h mgr.h \
		 func.h bfunc.h udf.h calc.h ast.h

include_HEADERS = fityk.h

