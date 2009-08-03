# $Id$

SUBDIRS = img
bin_PROGRAMS = fityk

EXTRA_DIST = winres.rc xyconvert.rc xybrowser.cpp

# tested only on MinGW
if OS_WIN32
AM_LDFLAGS = -mwindows
ms_win_res = winres.o
$(ms_win_res): winres.rc img/fityk.ico
	$(RC) -o $@ `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS)\
			    @WX_CPPFLAGS@ \
				    | sed -e 's/-mthreads//g' ` \
		    $<
HELP_DEF=
else
HELP_DEF= -DHELP_DIR=\"$(pkgdatadir)\"
endif  #OS_WIN32

fityk_SOURCES = cmn.cpp cmn.h gradient.cpp gradient.h fancyrc.cpp fancyrc.h \
		inputline.cpp inputline.h callback.h \
		listptxt.cpp listptxt.h dload.cpp dload.h pplot.cpp pplot.h \
		statbar.cpp statbar.h pane.cpp pane.h sidebar.cpp sidebar.h \
		dataedit.cpp dataedit.h defmgr.cpp defmgr.h \
		setdlg.cpp setdlg.h print.cpp print.h sdebug.cpp sdebug.h \
		plot.cpp plot.h aplot.cpp aplot.h mplot.cpp mplot.h \
		uplot.cpp uplot.h about.cpp about.h fitinfo.cpp fitinfo.h \
		dialogs.cpp dialogs.h frame.cpp frame.h app.cpp app.h

AM_CPPFLAGS = @WX_CPPFLAGS@ @CPPFLAGS_3RDPARTY@ @XYLIB_CPPFLAGS@ $(HELP_DEF)
AM_CXXFLAGS = @WX_CXXFLAGS_ONLY@
LDADD = ../libfityk.la @WX_LIBS@ $(ms_win_res)


inputline: sample_inputline.cpp inputline.cpp inputline.h callback.h
	`wx-config --cxx` `wx-config --cxxflags` -g -Wall -W -O0 -o $@ $< inputline.cpp `wx-config --libs`


if OS_WIN32
xyconvert_res = xyconvertres.o
$(xyconvert_res): xyconvert.rc img/xyconvert.ico
	$(RC) -o $@ `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS)\
			    @WX_CPPFLAGS@  | sed -e 's/-mthreads//g' ` $<
xyconvert = xyconvert.exe
else
xyconvert = xyconvert
endif

$(xyconvert): xybrowser.cpp uplot.cpp uplot.h $(xyconvert_res)
	$(CXX) -o $@ $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) -DXYCONVERT \
	$(AM_CXXFLAGS) $(CXXFLAGS) \
	$(AM_LDFLAGS) $(LDFLAGS) \
	$< $(srcdir)/uplot.cpp $(xyconvert_res) \
	../../3rdparty/xylib/xylib/.libs/libxy.a @WX_LIBS@ $(LIBS)


powdifpat: powdifpat.cpp cmn.o
	$(CXX) -o $@ $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) \
	-Icctbx/cctbx/include/ -Icctbx/scitbx/include/ \
	$(AM_CXXFLAGS) $(CXXFLAGS) \
	$< $(srcdir)/cmn.o \
	$(AM_LDFLAGS) $(LDFLAGS) -Lcctbx -lcctbx \
	../../3rdparty/xylib/xylib/.libs/libxy.a @WX_LIBS@ $(LIBS)

