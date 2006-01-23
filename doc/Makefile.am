# $Id$

dist_pkgdata_DATA = fityk_tips.txt fitykhelp.htb
dist_man_MANS = fityk.1
EXTRA_DIST = fitykhelp.xml hh-mod.xsl fitykhelp.html 
SUBDIRS = fitykhelp_img

all: fitykhelp.htb fitykhelp.html

fitykhelp.htb: fitykhelp.xml 
	rm -rf fh_tmp/
	mkdir fh_tmp
	mkdir fh_tmp/fitykhelp_img/
	cp -r $(srcdir)/fitykhelp_img/*.png fh_tmp/fitykhelp_img/
	xmlto -o fh_tmp -m $(srcdir)/hh-mod.xsl htmlhelp $(srcdir)/fitykhelp.xml
	cd fh_tmp/ && zip -r fitykhelp.zip .
	mv -f fh_tmp/fitykhelp.zip ./fitykhelp.htb 

fitykhelp.html: fitykhelp.xml 
	xmlto html-nochunks $(srcdir)/fitykhelp.xml 

