# $Id$

dist_pkgdata_DATA = fityk_tips.txt fitykhelp.htb
dist_man_MANS = fityk.1
EXTRA_DIST = fitykhelp.xml cmdidx.xsl hh-mod.xsl fitykhelp.html 
SUBDIRS = fitykhelp_img

all: fitykhelp.htb fitykhelp.html

# fitykhelp_with_idx.xml is produced twice, but it is not a separate target,
# to not have to be include it in EXTRA_DIST
fitykhelp.htb: fitykhelp.xml cmdidx.xsl
	xsltproc -o fitykhelp_with_idx.xml --nonet cmdidx.xsl fitykhelp.xml 
	rm -rf fitykhelp_tmp/
	mkdir fitykhelp_tmp
	mkdir fitykhelp_tmp/fitykhelp_img/
	cp -r fitykhelp_img/*.png fitykhelp_tmp/fitykhelp_img/
	xmlto -o fitykhelp_tmp -m hh-mod.xsl htmlhelp fitykhelp_with_idx.xml 
	cd fitykhelp_tmp/ && zip -r fitykhelp.zip .
	mv -f fitykhelp_tmp/fitykhelp.zip ./fitykhelp.htb 

fitykhelp.html: fitykhelp.xml cmdidx.xsl
	xsltproc -o fitykhelp_with_idx.xml --nonet cmdidx.xsl fitykhelp.xml 
	xmlto html-nochunks fitykhelp_with_idx.xml 
	mv -f fitykhelp_with_idx.html fitykhelp.html 

