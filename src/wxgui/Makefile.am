
bin_PROGRAMS = fityk

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
HELP_DEF= -DHELP_DIR=\"$(pkgdatadir)\"
endif  #OS_WIN32

noinst_LIBRARIES = libceria.a
libceria_a_SOURCES = ceria.cpp ceria.h atomtables.c atomtables.h \
		     sgtables.c sgtables.h

fityk_SOURCES = cmn.cpp cmn.h gradient.cpp gradient.h fancyrc.cpp fancyrc.h \
		inputline.cpp inputline.h listptxt.cpp listptxt.h \
		dload.cpp dload.h xybrowser.cpp xybrowser.h \
		plotpane.cpp plotpane.h textpane.cpp textpane.h \
		statbar.cpp statbar.h sidebar.cpp sidebar.h \
		history.cpp history.h datatable.cpp datatable.h \
		dataedit.cpp dataedit.h defmgr.cpp defmgr.h \
		setdlg.cpp setdlg.h print.cpp print.h sdebug.cpp sdebug.h \
		plot.cpp plot.h aplot.cpp aplot.h mplot.cpp mplot.h \
		uplot.cpp uplot.h about.cpp about.h fitinfo.cpp fitinfo.h \
		dialogs.cpp dialogs.h exportd.cpp exportd.h bgm.cpp bgm.h \
		frame.cpp frame.h app.cpp app.h \
		powdifpat.cpp powdifpat.h sgchooser.cpp sgchooser.h


fityk_CPPFLAGS = $(WX_CPPFLAGS) $(HELP_DEF)
fityk_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
fityk_LDADD = ../libfityk.la libceria.a -lxy $(WX_LIBS) $(fityk_res)

if XYCONVERT
bin_PROGRAMS += xyconvert
xyconvert_SOURCES = xybrowser.cpp xybrowser.h uplot.cpp uplot.h cmn.cpp cmn.h
xyconvert_CPPFLAGS = $(WX_CPPFLAGS) -DXYCONVERT
xyconvert_LDADD = $(WX_LIBS) -lxy $(xyconvert_res)
endif

inputline: sample_inputline.cpp inputline.cpp inputline.h
	`wx-config --cxx` `wx-config --cxxflags` -g -Wall -W -O0 -o $@ \
	    $< $(srcdir)/inputline.cpp `wx-config --libs`

if POWDIFPAT
bin_PROGRAMS += powdifpat
powdifpat_SOURCES = powdifpat.cpp powdifpat.h sgchooser.cpp sgchooser.h \
		    uplot.cpp uplot.h cmn.cpp cmn.h fancyrc.cpp fancyrc.h
powdifpat_CPPFLAGS = $(WX_CPPFLAGS) -DSTANDALONE_POWDIFPAT
powdifpat_LDADD = libceria.a $(WX_LIBS) -lxy $(powdifpat_res)
endif

xyconvert.html: xyconvert.rst
	rst2html --stylesheet-path=$$HOME/xylib/trunk/web.css $< $@


## to find icons not listed here use (in bash):
## for i in img/*.xpm img/*.h; do [ `grep -c $i Makefile.am` -eq 0 ] && ls $i; done

EXTRA_DIST = \
winres.rc xyconvert.rc powdifpat.rc \
\
img/active_mode.xpm \
img/addpeak_mode.xpm \
img/add_peak.xpm \
img/add.xpm \
img/bg_mode.xpm \
img/close.xpm \
img/colorsel.xpm \
img/color.xpm \
img/cont_fit.xpm \
img/convert.xpm \
img/copyfunc.xpm \
img/edit_data.xpm \
img/editf.xpm \
img/exec_down.xpm \
img/exec_selected.xpm \
img/filter.xpm \
img/fityk.xpm \
img/goto.xpm \
img/lock.xpm \
img/lock_open.xpm \
img/manual.xpm \
img/open.xpm \
img/open_data_custom.xpm \
img/open_data.xpm \
img/rename.xpm \
img/right_pane.xpm \
img/run_fit.xpm \
img/run_script.xpm \
img/save.xpm \
img/save_as.xpm \
img/save_data.xpm \
img/save_script.xpm \
img/shiftup.xpm \
img/dpsize.xpm \
img/strip_bg.xpm \
img/sum.xpm \
img/undo_fit.xpm \
img/unused.xpm \
img/zoom_all.xpm \
img/zoom_left.xpm \
img/zoom_mode.xpm \
img/zoom_prev.xpm \
img/zoom_right.xpm \
img/zoom_up.xpm \
img/zoom_vert.xpm \
img/zshift.xpm \
\
img/eq_fwhm.h \
img/eq_shape.h \
\
img/book16.h \
img/editor16.h \
img/export16.h \
img/fileopen16.h \
img/filereload16.h \
img/filesaveas16.h \
img/function16.h \
img/image16.h \
img/info16.h \
img/preferences16.h \
img/recordmacro16.h \
img/redo16.h \
img/reload16.h \
img/revert16.h \
img/run16.h \
img/runmacro16.h \
img/stopmacro16.h \
img/undo16.h \
img/web16.h \
img/zoom-fit16.h \
\
img/mouse16.h \
img/sbprefs.h \
img/ok24.h \
\
img/correction.h \
img/info32.h \
img/peak32.h \
img/radiation32.h \
img/rubik32.h \
img/run32.h \
img/sizes32.h \
\
img/fityk96.h \
\
img/fityk.ico \
\
img/xyconvert16.xpm \
img/xyconvert48.xpm \
img/xyconvert.ico \
\
img/powdifpat16.xpm \
img/powdifpat48.xpm \
img/powdifpat.ico

