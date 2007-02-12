# $Id$

dist_pkgdata_DATA = fityk_tips.txt fitykhelp.htb 
dist_man_MANS = fityk.1
EXTRA_DIST = fitykhelp.xml hh-mod.xsl fitykhelp.html html.css
SUBDIRS = fitykhelp_img

all: fitykhelp.htb fitykhelp.html

fitykhelp.htb: fitykhelp.xml 
	rm -rf fh_tmp/
	mkdir fh_tmp
	mkdir fh_tmp/fitykhelp_img/
	cp -r $(srcdir)/fitykhelp_img/*.png fh_tmp/fitykhelp_img/
	cp $(srcdir)/html.css fh_tmp/
	xmlto -o fh_tmp -m $(srcdir)/hh-mod.xsl htmlhelp $(srcdir)/fitykhelp.xml
	cd fh_tmp/ && zip -r fitykhelp.zip .
	mv -f fh_tmp/fitykhelp.zip ./fitykhelp.htb 

fitykhelp.html: fitykhelp.xml 
	xmlto -m $(srcdir)/hh-mod.xsl html-nochunks $(srcdir)/fitykhelp.xml 

# Apache FOP in needed, setting JAVA_HOME may be necessary
#JAVA_HOME=/usr/lib/j2re1.5-sun
fitykhelp.pdf: fitykhelp.xml fo.xsl
	xmlto -m fo.xsl fo $(srcdir)/fitykhelp.xml
	fop fitykhelp.fo fitykhelp.pdf

