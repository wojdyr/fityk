# $Id: $

SUBDIRS = img 

EXTRA_DIST = winres.rc 

# tested only on MinGW 
if OS_WIN32
AM_LDFLAGS = -mwindows 
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

AM_SOURCES = cmn.cpp cmn.h gradient.cpp gradient.h fancyrc.cpp fancyrc.h \
		inputline.cpp inputline.h callback.h \
		listptxt.cpp listptxt.h dload.cpp dload.h \
		statbar.cpp statbar.h pane.cpp pane.h sidebar.cpp sidebar.h \
		dataedit.cpp dataedit.h defmgr.cpp defmgr.h \
		setdlg.cpp setdlg.h print.cpp print.h sdebug.cpp sdebug.h \
		plot.cpp plot.h aplot.cpp aplot.h mplot.cpp mplot.h \
		dialogs.cpp dialogs.h gui.cpp gui.h 


AM_CPPFLAGS = @WX_CPPFLAGS@ @CPPFLAGS_3RDPARTY@ 
AM_CXXFLAGS = @WX_CXXFLAGS_ONLY@ 

AM_LDADD = ../libfityk.la @WX_LIBS@ $(ms_win_res)

bin_PROGRAMS = @FITYK@ 
EXTRA_PROGRAMS = fityk 

inputline: sample_inputline.cpp inputline.cpp inputline.h callback.h
	`wx-config --cxx` `wx-config --cxxflags` -g -Wall -W -O0 -o $@ $< inputline.cpp `wx-config --libs`

