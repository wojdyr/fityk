# $Id: $

SUBDIRS = img 

EXTRA_DIST = winres.rc 

# tested only on MinGW 
if OS_WIN32
fityk_LDFLAGS = -mwindows 
ms_win_res = winres.o
# windres has "--include-dir" instead of "-I" 
# [update: now windres accepts also -I, so this sed thing in not necessary]
$(ms_win_res): winres.rc img/fityk.ico 
	$(RC) -o $@ `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS)\
			    @WX_CPPFLAGS@ \
				    | sed -e 's/-I/--include-dir /g' \
				    | sed -e 's/-mthreads//g' ` \
		    $<
else
INCLUDES = -DHELP_DIR=\"$(pkgdatadir)\"
endif  #OS_WIN32

fityk_SOURCES = cmn.cpp cmn.h gradient.cpp gradient.h fancyrc.cpp fancyrc.h \
		listptxt.cpp listptxt.h \
		wx_dlg.cpp wx_gui.cpp wx_IO.cpp wx_plot.cpp \
		wx_mplot.cpp wx_pane.cpp wx_fdlg.cpp \
		wx_dlg.h wx_gui.h wx_plot.h wx_mplot.h wx_pane.h \
		wx_fdlg.h 


AM_CPPFLAGS = @WX_CPPFLAGS@ 
AM_CXXFLAGS = -Wall @WX_CXXFLAGS_ONLY@ 

fityk_LDADD = ../libfityk.a @WX_LIBS@ $(ms_win_res)

bin_PROGRAMS = @FITYK@ 
EXTRA_PROGRAMS = fityk 

