# $Id$
EXTRA_DIST = scanner.ll parser.yy winres.rc 
SUBDIRS = img 

# tested only on MinGW 
if OS_WIN32
fityk_LDFLAGS = -mwindows
ms_win_res = winres.o
$(ms_win_res): winres.rc
	windres -o $@ --include-dir $(srcdir) \
	--include-dir /usr/local/include --include-dir /usr/include/ \
	-D__WIN32__ -D__WIN95__ -D__GNUWIN32__ $<
# there is a problem with windres syntax (--include-dir instead of -I).
# perhaps 'sed/ -I/ --include-dir /g' should be run 
endif

bin_PROGRAMS = @FITYK@ @CFITYK@ 
EXTRA_PROGRAMS = fityk cfityk 
common_sources = other.cpp scanner.cpp parser.cpp common.cpp crystal.cpp \
		 data.cpp dotset.cpp ffunc.cpp file_IO.cpp fzgbase.cpp \
		 GAfit.cpp gfunc.cpp LMfit.cpp manipul.cpp NMfit.cpp \
		 sum.cpp v_fit.cpp voigt.cpp pcore.cpp fileroutines.cpp 
gui_sources = wx_dlg.cpp wx_gui.cpp wx_IO.cpp wx_plot.cpp wx_pane.cpp \
	         wx_common.h
fityk_SOURCES = $(common_sources) $(gui_sources) 
cfityk_SOURCES = $(common_sources) u_gnuplot.cpp u_main.cpp u_rl_IO.cpp 
common_ldadd = ##ext.a @FLIBS@  
fityk_LDADD = $(common_ldadd) @WX_LIBS_DS@ $(ms_win_res)
cfityk_LDADD = $(common_ldadd) @READLINE_LIBS@ -lm
noinst_HEADERS = common.h crystal.h data.h dotset.h ffunc.h fzgbase.h \
		 GAfit.h gfunc.h LMfit.h manipul.h NMfit.h other.h pag.h \
		 sum.h v_fit.h v_IO.h voigt.h pcore.h fileroutines.h \
		 u_gnuplot.h wx_dlg.h wx_gui.h wx_plot.h wx_pane.h \
		 parser.tab.hh

CXXFLAGS_BASE = -W -Wall -fno-rtti -fno-exceptions -pipe
AM_CXXFLAGS = $(CXXFLAGS_BASE)
# gcc 2.95 has too small template-depth
other.o: AM_CXXFLAGS = $(CXXFLAGS_BASE) -ftemplate-depth-30 
$(gui_sources:.cpp=.o): AM_CXXFLAGS = $(CXXFLAGS_BASE) @WX_CPPFLAGS@ @WX_CXXFLAGS_ONLY@ 

##TODO:
parser.cpp: parser.yy
	bison -p ip -d parser.yy && \
	mv parser.tab.cc parser.cpp

scanner.cpp: scanner.ll parser.cpp 
	flex scanner.ll && \
	sed "s/#include <unistd.h>/\/\/&/" lex.ip.c >scanner.cpp && \
	rm -f lex.ip.c

