ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = README.md fityk.iss fityk.url \
             doc/fityk-manual.rst doc/intro.rst doc/getstarted.rst \
             doc/data.rst doc/model.rst doc/fit.rst \
             doc/script.rst doc/ref.rst \
             doc/Makefile doc/conf.py doc/fityk_ext.py \
             doc/img doc/fitheme doc/html

SUBDIRS = fityk
if GUI_DIR
SUBDIRS += wxgui
endif
if CLI_DIR
bin_PROGRAMS = cli/cfityk
endif

# for Linux desktop
icon32dir = $(datadir)/icons/hicolor/32x32/apps
iconscdir = $(datadir)/icons/hicolor/scalable/apps
desktopdir = $(datadir)/applications
mimepkgdir = $(datadir)/mime/packages
appdatadir = $(datadir)/appdata

dist_icon32_DATA = fityk.png
dist_iconsc_DATA = fityk.svg
dist_desktop_DATA = fityk.desktop
dist_mimepkg_DATA = fityk.xml
dist_appdata_DATA = fityk.appdata.xml

# ---  cli/ ---
cli_cfityk_SOURCES = cli/gnuplot.cpp cli/main.cpp cli/gnuplot.h
cli_cfityk_LDADD = fityk/libfityk.la $(READLINE_LIBS)

# ---  tests/ ---
TESTS = tests/gradient tests/guess tests/psvoigt tests/num tests/lua
check_LIBRARIES = tests/libcatch.a
tests_libcatch_a_SOURCES = tests/catch.cpp tests/catch.hpp
tests_gradient_SOURCES = tests/gradient.cpp
tests_gradient_LDADD = fityk/libfityk.la tests/libcatch.a
tests_gradient_LDFLAGS = -no-install
tests_guess_SOURCES = tests/guess.cpp
tests_guess_LDADD = fityk/libfityk.la tests/libcatch.a
tests_guess_LDFLAGS = -no-install
tests_psvoigt_SOURCES = tests/psvoigt.cpp
tests_psvoigt_LDADD = fityk/libfityk.la tests/libcatch.a
tests_psvoigt_LDFLAGS = -no-install
tests_num_SOURCES = tests/num.cpp
tests_num_LDADD = fityk/libfityk.la tests/libcatch.a
tests_num_LDFLAGS = -no-install
tests_lua_SOURCES = tests/lua.cpp
tests_lua_LDADD = fityk/libfityk.la tests/libcatch.a
tests_lua_LDFLAGS = -no-install
check_PROGRAMS = $(TESTS)
if ! OS_WIN32
check_PROGRAMS += tests/mpfit_deriv
tests_mpfit_deriv_SOURCES = tests/mpfit_deriv.cpp
tests_mpfit_deriv_LDADD = fityk/libfityk.la
tests_mpfit_deriv_LDFLAGS = -no-install
endif
dist_noinst_SCRIPTS = tests/test_syntax.fit \
      tests/test_nist.py tests/test_guess.py tests/test_model.py \
      tests/test_tranform.py tests/test_data_load.py tests/test_info.py

# --- samples/ ---

samplesdir = $(pkgdatadir)/samples
dist_samples_DATA = samples/nacl01.fit samples/nacl01.dat \
		    samples/SiC_Zn.fit samples/SiC_Zn.dat  \
		    samples/enso.fit samples/enso.dat \
		    samples/read-shockley.fit \
		    samples/hello.c samples/hello.cc \
		    samples/hello.py samples/hello.lua samples/hello.pl \
		    samples/hello.rb samples/hello.java \
		    samples/cfityk.py \
		    samples/README

check_PROGRAMS += samples/hello samples/helloc
samples_hello_SOURCES = samples/hello.cc
samples_hello_CPPFLAGS = -I$(srcdir)
samples_hello_LDADD = fityk/libfityk.la
samples_helloc_SOURCES = samples/hello.c
samples_helloc_CPPFLAGS = -I$(srcdir)
samples_helloc_LDADD = fityk/libfityk.la
# workaround to link helloc with C++ linker
nodist_EXTRA_samples_helloc_SOURCES = dummy.cxx

check-cfityk:
	cli/cfityk -q -c "= F:get_info('version')..'\n'..F:get_info('compiler')"
	cli/cfityk -n -q $(srcdir)/samples/enso.fit
	cli/cfityk -n -q $(srcdir)/samples/read-shockley.fit
	cli/cfityk -n -q $(srcdir)/samples/nacl01.fit
	cli/cfityk -n -q $(srcdir)/samples/SiC_Zn.fit
	cli/cfityk -n -q $(srcdir)/samples/hello.lua
	rm -f tmp_save.fit
	cli/cfityk -n -q $(srcdir)/tests/test_syntax.fit

check-python:
	PYTHONPATH=fityk/swig/.libs:$(srcdir)/fityk/swig/ \
	   $(PYTHON) -m unittest discover -s $(srcdir)/tests/

check-help:
	echo '$$ fityk -h'   > $(srcdir)/doc/help-message.txt
	wxgui/fityk -h 2>&1 | sed s/lt-fi/fi/ >> $(srcdir)/doc/help-message.txt
	echo -e '\n$$ cfityk -h' >> $(srcdir)/doc/help-message.txt
	cli/cfityk -h       >> $(srcdir)/doc/help-message.txt

check-scripts: check-cfityk check-python check-help

# --- doc/ ---

dist_man_MANS = doc/fityk.1

install-data-hook:
	@$(NORMAL_INSTALL)
	$(MKDIR_P) "$(DESTDIR)$(pkgdatadir)/html"
	cp -fR $(srcdir)/doc/html/* "$(DESTDIR)$(pkgdatadir)/html" && \
	    chmod -R u+w "$(DESTDIR)$(pkgdatadir)/html" ||:

uninstall-local:
	@$(NORMAL_UNINSTALL)
	cd "$(DESTDIR)$(pkgdatadir)" && rm -rf html
