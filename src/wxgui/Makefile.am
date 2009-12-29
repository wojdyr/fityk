# $Id$

SUBDIRS = img
bin_PROGRAMS = fityk

EXTRA_DIST = winres.rc xyconvert.rc powdifpat.rc

# tested only on MinGW
if OS_WIN32
AM_LDFLAGS = -mwindows
RC_FLAGS_ALL=$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) $(WX_CPPFLAGS)
RC_FLAGS=`echo $(RC_FLAGS_ALL) | sed -e 's/-mthreads//g' `
fityk_res = winres.o
$(fityk_res): winres.rc img/fityk.ico
	$(RC) -o $@ $(RC_FLAGS) $<
HELP_DEF=
xyconvert_res = xyconvertres.o
$(xyconvert_res): xyconvert.rc img/xyconvert.ico
	$(RC) -o $@ $(RC_FLAGS) $<

powdifpat_res = powdifpatres.o
$(powdifpat_res): powdifpat.rc img/powdifpat.ico
	$(RC) -o $@ $(RC_FLAGS) $<
else
HELP_DEF= -DHELP_DIR=\"$(docdir)\"
endif  #OS_WIN32

noinst_LIBRARIES = libceria.a
libceria_a_SOURCES = ceria.cpp ceria.h atomtables.c atomtables.h \
		     sgtables.c sgtables.h

fityk_SOURCES = cmn.cpp cmn.h gradient.cpp gradient.h fancyrc.cpp fancyrc.h \
		inputline.cpp inputline.h callback.h \
		listptxt.cpp listptxt.h dload.cpp dload.h pplot.cpp pplot.h \
		statbar.cpp statbar.h pane.cpp pane.h sidebar.cpp sidebar.h \
		history.cpp history.h datatable.cpp datatable.h \
		dataedit.cpp dataedit.h defmgr.cpp defmgr.h \
		setdlg.cpp setdlg.h print.cpp print.h sdebug.cpp sdebug.h \
		plot.cpp plot.h aplot.cpp aplot.h mplot.cpp mplot.h \
		uplot.cpp uplot.h about.cpp about.h fitinfo.cpp fitinfo.h \
		dialogs.cpp dialogs.h frame.cpp frame.h app.cpp app.h \
		powdifpat.cpp powdifpat.h sgchooser.cpp sgchooser.h


fityk_CPPFLAGS = $(WX_CPPFLAGS) $(CPPFLAGS_3RDPARTY) $(XYLIB_CPPFLAGS) $(HELP_DEF)
fityk_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
fityk_LDADD = ../libfityk.la libceria.a $(WX_LIBS) $(fityk_res)

if XYCONVERT
bin_PROGRAMS += xyconvert
xyconvert_SOURCES = xybrowser.cpp uplot.cpp uplot.h cmn.cpp cmn.h
xyconvert_CPPFLAGS = $(WX_CPPFLAGS) $(CPPFLAGS_3RDPARTY) $(XYLIB_CPPFLAGS) -DXYCONVERT
xyconvert_LDADD = $(WX_LIBS) $(XYLIB_LDADD) $(xyconvert_res)
endif

inputline: sample_inputline.cpp inputline.cpp inputline.h callback.h
	`wx-config --cxx` `wx-config --cxxflags` -g -Wall -W -O0 -o $@ $< inputline.cpp `wx-config --libs`


if POWDIFPAT
bin_PROGRAMS += powdifpat
powdifpat_SOURCES = powdifpat.cpp powdifpat.h sgchooser.cpp sgchooser.h \
		    uplot.cpp uplot.h cmn.cpp cmn.h fancyrc.cpp fancyrc.h
powdifpat_CPPFLAGS = $(WX_CPPFLAGS) $(XYLIB_CPPFLAGS) -DSTANDALONE_POWDIFPAT
powdifpat_LDADD = libceria.a $(WX_LIBS) $(XYLIB_LDADD) $(powdifpat_res)
endif

