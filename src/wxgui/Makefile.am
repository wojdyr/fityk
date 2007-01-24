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
		listptxt.cpp listptxt.h dload.cpp dload.h \
		statbar.cpp statbar.h pane.cpp pane.h sidebar.cpp sidebar.h \
		dataedit.cpp dataedit.h defmgr.cpp defmgr.h \
		setdlg.cpp setdlg.h print.cpp print.h sdebug.cpp sdebug.h \
		plot.cpp plot.h aplot.cpp aplot.h mplot.cpp mplot.h \
		dlgs.cpp dlgs.h gui.cpp gui.h wx_IO.cpp 


AM_CPPFLAGS = @WX_CPPFLAGS@ 
AM_CXXFLAGS = -Wall @WX_CXXFLAGS_ONLY@ 

fityk_LDADD = ../libfityk.a @WX_LIBS@ $(ms_win_res)

bin_PROGRAMS = @FITYK@ 
EXTRA_PROGRAMS = fityk 

