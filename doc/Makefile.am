# $Id$

dist_pkgdata_DATA = fityk_tips.txt fitykhelp.htb
dist_man_MANS = fityk.1
EXTRA_DIST = fitykhelp.xml hh-mod.xsl fitykhelp.html 
SUBDIRS = fitykhelp_img

all: fitykhelp.htb fitykhelp.html

fitykhelp.htb: fitykhelp.xml 
	rm -rf fitykhelp_tmp/
	mkdir fitykhelp_tmp
	mkdir fitykhelp_tmp/fitykhelp_img/
	cp -r fitykhelp_img/*.png fitykhelp_tmp/fitykhelp_img/
	xmlto -o fitykhelp_tmp -m hh-mod.xsl htmlhelp fitykhelp.xml 
	cd fitykhelp_tmp/ && zip -r fitykhelp.zip .
	mv -f fitykhelp_tmp/fitykhelp.zip ./fitykhelp.htb 

fitykhelp.html: fitykhelp.xml 
	xmlto html-nochunks fitykhelp.xml 

