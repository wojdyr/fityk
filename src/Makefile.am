# $Id$
SUBDIRS = . $(SRCSUBDIRS)
DIST_SUBDIRS = cli wxgui

lib_LTLIBRARIES = libfityk.la

libfityk_la_LDFLAGS = $(LIBRARY_VERSION_FLAG)
libfityk_la_LIBADD = -lxy

libfityk_la_SOURCES = logic.cpp view.cpp lexer.cpp eparser.cpp cparser.cpp \
		 runner.cpp info.cpp common.cpp data.cpp var.cpp mgr.cpp \
		 tplate.cpp func.cpp udf.cpp bfunc.cpp calc.cpp ast.cpp vm.cpp \
		 settings.cpp ui.cpp GAfit.cpp LMfit.cpp guess.cpp NMfit.cpp \
		 model.cpp fit.cpp voigt.cpp numfuncs.cpp fityk.cpp \
		 \
                 logic.h view.h lexer.h eparser.h cparser.h \
		 runner.h info.h common.h data.h var.h mgr.h \
		 tplate.h func.h udf.h bfunc.h calc.h ast.h vm.h \
		 settings.h ui.h GAfit.h LMfit.h guess.h NMfit.h \
		 model.h fit.h voigt.h numfuncs.h

include_HEADERS = fityk.h

