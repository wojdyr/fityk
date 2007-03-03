# $Id$

dist_pkgdata_DATA = fityk_tips.txt fitykhelp.htb 
dist_man_MANS = fityk.1
EXTRA_DIST = fitykhelp.xml hh-mod.xsl fitykhelp.html html.css fo.xsl
SUBDIRS = fitykhelp_img

all: fitykhelp.htb fitykhelp.html

fitykhelp.htb: $(srcdir)/fitykhelp.xml 
	rm -rf fh_tmp/
	mkdir fh_tmp
	mkdir fh_tmp/fitykhelp_img/
	cp -r $(srcdir)/fitykhelp_img/*.png fh_tmp/fitykhelp_img/
	cp $(srcdir)/html.css fh_tmp/
	xmlto -o fh_tmp -m $(srcdir)/hh-mod.xsl htmlhelp $(srcdir)/fitykhelp.xml
	echo "fitykhelp_img/note.png" >> fh_tmp/htmlhelp.hhp 
	cd fh_tmp/ && zip -r fitykhelp.zip .
	mv -f fh_tmp/fitykhelp.zip ./fitykhelp.htb 

fitykhelp.html: $(srcdir)/fitykhelp.xml 
	xmlto -m $(srcdir)/hh-mod.xsl html-nochunks $(srcdir)/fitykhelp.xml 

html-chunks: 
	xmlto -m $(srcdir)/hh-mod.xsl html $(srcdir)/fitykhelp.xml 

# Apache FOP in needed, setting JAVA_HOME may be necessary
#JAVA_HOME=/usr/lib/j2re1.5-sun
# sed is used to make the images smaller in pdf (recommended :))
# pdftk is used to get rid of 2nd and 3rd page (optional :))
fitykhelp.pdf: fitykhelp.xml fo.xsl
	sed -e 's/\.png"\/>/\.png" scale="66.66"\/>/g' fitykhelp.xml >fitykhelp_.xml
	xmlto -m $(srcdir)/fo.xsl fo $(srcdir)/fitykhelp_.xml
	fop fitykhelp_.fo fitykhelp_.pdf
	pdftk fitykhelp_.pdf cat 1 4-end output fitykhelp.pdf

