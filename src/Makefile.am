# $Id$
EXTRA_DIST = winres.rc 
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
common_sources = logic.cpp cmd3.cpp cmd2.cpp cmd.cpp common.cpp \
		 var.cpp func.cpp calc.cpp \
		 data.cpp datatrans.cpp datatrans2.cpp datatrans3.cpp \
		 datatrans4.cpp \
		 settings.cpp ui.cpp GAfit.cpp LMfit.cpp guess.cpp NMfit.cpp \
		 sum.cpp fit.cpp voigt.cpp fileroutines.cpp \
		 numfuncs.cpp nmath.cpp
gui_sources = wx_common.cpp wx_dlg.cpp wx_gui.cpp wx_IO.cpp wx_plot.cpp \
	      wx_mplot.cpp wx_pane.cpp wx_fdlg.cpp
fityk_SOURCES = $(common_sources) $(gui_sources) 
cfityk_SOURCES = $(common_sources) u_gnuplot.cpp u_main.cpp 
common_ldadd = ##none
fityk_LDADD = $(common_ldadd) @WX_LIBS@ $(ms_win_res)
cfityk_LDADD = $(common_ldadd) @READLINE_LIBS@ -lm
noinst_HEADERS = common.h data.h datatrans.h datatrans2.h datatrans3.h \
		 datatrans4.h \
		 settings.h GAfit.h LMfit.h guess.h NMfit.h logic.h \
		 sum.h fit.h ui.h voigt.h fileroutines.h \
		 u_gnuplot.h \
		 wx_common.h wx_dlg.h wx_gui.h wx_plot.h wx_mplot.h wx_pane.h \
		 wx_fdlg.h \
		 numfuncs.h cmd.h cmd2.h cmd3.h var.h func.h calc.h \
		 optional_suffix.h

# -ftemplate-depth-50 is for gcc 2.95 
AM_CXXFLAGS = -Wall -ftemplate-depth-50 
$(gui_sources:.cpp=.o): AM_CXXFLAGS += @WX_CPPFLAGS@ @WX_CXXFLAGS_ONLY@ 

