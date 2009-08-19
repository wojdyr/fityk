# $Id$

dist_pkgdata_DATA = html
dist_man_MANS = fityk.1

INPUT_FILES = appendix.rst fityk-manual.rst index.rst ref.rst \
	      extensions.rst getstarted.rst intro.rst \
	      fityk.css layout.html conf.py \
	      fityk.ico fityk076.png project-support.jpg \
	      wxwidgets_powered.png fityk-banner.png unipress-button.png

EXTRA_DIST = $(INPUT_FILES)

# ATM I'm using sphinx from 1.0-hg with a few hacks.
# hopefully in the future these hacks won't be necessary
SPHINX = sphinx-build -a -d ./doctrees/

all: html/fityk-manual.html

html/fityk-manual.html: $(INPUT_FILES)
	$(SPHINX) -b html . html
	rm -f html/objects.inv
	sed -i -e 's/fityk — free peak fitting software &mdash; Fityk 0.8.9 manual/fityk \&mdash; free peak fitting software/' html/index.html

pdf:
	rm -rf doctrees/
	$(SPHINX) -b latex . latex
	cd latex/ && make all-pdf

