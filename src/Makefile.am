# $Id$
SUBDIRS = . $(SRCSUBDIRS)
DIST_SUBDIRS = cli wxgui

lib_LTLIBRARIES = libfityk.la

libfityk_la_LDFLAGS = $(LIBRARY_VERSION_FLAG)
libfityk_la_LIBADD = -lxy

libfityk_la_SOURCES = logic.cpp view.cpp lexer.cpp eparser.cpp cparser.cpp \
		 runner.cpp info.cpp \
		 cmd3.cpp cmd2.cpp cmd.cpp common.cpp \
		 var.cpp mgr.cpp func.cpp udf.cpp bfunc.cpp calc.cpp ast.cpp \
		 data.cpp datatrans.cpp datatrans2.cpp datatrans3.cpp \
		 datatrans4.cpp \
		 settings.cpp ui.cpp GAfit.cpp LMfit.cpp guess.cpp NMfit.cpp \
		 model.cpp fit.cpp voigt.cpp \
		 numfuncs.cpp fityk.cpp \
		 \
                 common.h data.h lexer.h eparser.h cparser.h runner.h \
		 datatrans.h datatrans2.h datatrans3.h datatrans4.h info.h \
		 settings.h GAfit.h LMfit.h guess.h NMfit.h logic.h view.h \
		 model.h fit.h ui.h voigt.h \
		 numfuncs.h cmd.h cmd2.h cmd3.h var.h mgr.h \
		 func.h bfunc.h udf.h calc.h ast.h optional_suffix.h

include_HEADERS = fityk.h

