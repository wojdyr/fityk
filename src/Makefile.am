# $Id$
EXTRA_DIST = scanner.ll parser.yy winres.rc 
SUBDIRS = img 

# tested only on MinGW 
if OS_WIN32
fityk_LDFLAGS = -mwindows 
ms_win_res = winres.o
# windres has "--include-dir" instead of "-I" 
$(ms_win_res): winres.rc img/fityk.ico 
	$(RC) -o $@ `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS)\
			    @WX_CPPFLAGS@ \
				    | sed -e 's/-I/--include-dir /g' \
				    | sed -e 's/-mthreads//g' ` \
		    $<
else
INCLUDES = -DHELP_DIR=\"$(pkgdatadir)\"
endif  #OS_WIN32


bin_PROGRAMS = @FITYK@ @CFITYK@ 
EXTRA_PROGRAMS = fityk cfityk 
common_sources = cmd.cpp other.cpp scanner.cpp parser.cpp common.cpp \
		 crystal.cpp \
		 data.cpp datatrans.cpp dotset.cpp ffunc.cpp fzgbase.cpp \
		 ui.cpp GAfit.cpp gfunc.cpp LMfit.cpp manipul.cpp NMfit.cpp \
		 sum.cpp v_fit.cpp voigt.cpp pcore.cpp fileroutines.cpp \
		 numfuncs.cpp
gui_sources = wx_dlg.cpp wx_gui.cpp wx_IO.cpp wx_plot.cpp wx_mplot.cpp \
	      wx_pane.cpp wx_common.h
fityk_SOURCES = $(common_sources) $(gui_sources) 
cfityk_SOURCES = $(common_sources) u_gnuplot.cpp u_main.cpp 
common_ldadd = ##none
fityk_LDADD = $(common_ldadd) @WX_LIBS_DS@ $(ms_win_res)
cfityk_LDADD = $(common_ldadd) @READLINE_LIBS@ -lm
noinst_HEADERS = common.h crystal.h data.h datatrans.h dotset.h ffunc.h \
		 fzgbase.h GAfit.h gfunc.h LMfit.h manipul.h NMfit.h other.h \
		 pag.h sum.h v_fit.h ui.h voigt.h pcore.h fileroutines.h \
		 u_gnuplot.h wx_dlg.h wx_gui.h wx_plot.h wx_mplot.h wx_pane.h \
		 numfuncs.h parser.tab.hh cmd.h

# -ftemplate-depth-50 is for gcc 2.95 
AM_CXXFLAGS = -Wall -ftemplate-depth-50 
$(gui_sources:.cpp=.o): AM_CXXFLAGS += @WX_CPPFLAGS@ @WX_CXXFLAGS_ONLY@ 

##TODO:
parser.cpp: $(srcdir)/parser.yy
	bison -p ip -d $(srcdir)/parser.yy && \
	mv parser.tab.cc parser.cpp

scanner.cpp: $(srcdir)/scanner.ll $(srcdir)/parser.cpp 
	flex $(srcdir)/scanner.ll && \
	sed "s/#include <unistd.h>/\/\/&/" lex.ip.c >scanner.cpp && \
	rm -f lex.ip.c

