
data_DATA = fityk_tips.txt fitykhelp.htb
EXTRA_DIST = $(data_DATA) fitykhelp.xml cmdidx.xsl fitykhelp.html fitykhelp_img/

all: fitykhelp.htb fitykhelp.html

fitykhelp.htb: fitykhelp.xml cmdidx.xsl
	export SGML_CATALOG_FILES=/usr/share/sgml/docbook/xml-dtd-4.1.2-1.0-8/
	xsltproc -o fitykhelp_with_idx.xml --nonet cmdidx.xsl fitykhelp.xml 
	rm -rf fitykhelp_tmp/
	mkdir fitykhelp_tmp
	mkdir fitykhelp_tmp/fitykhelp_img/
	cp -r fitykhelp_img/*.png fitykhelp_tmp/fitykhelp_img/
	xmlto -o fitykhelp_tmp htmlhelp fitykhelp_with_idx.xml 
	sed "s/Charset.*\n//" fitykhelp_tmp/htmlhelp.hhp >fitykhelp_tmp/xx.hhp 
	mv -f fitykhelp_tmp/xx.hhp fitykhelp_tmp/htmlhelp.hhp
	cd fitykhelp_tmp/ && zip -r fitykhelp.zip .
	mv -f fitykhelp_tmp/fitykhelp.zip ./fitykhelp.htb 

fitykhelp.html: fitykhelp.xml cmdidx.xsl
	export SGML_CATALOG_FILES=/usr/share/sgml/docbook/xml-dtd-4.1.2-1.0-8/
	xsltproc -o fitykhelp_with_idx.xml --nonet cmdidx.xsl fitykhelp.xml 
	xmlto html-nochunks fitykhelp_with_idx.xml 
	mv -f fitykhelp_with_idx.html fitykhelp.html 

