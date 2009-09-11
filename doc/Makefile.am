# $Id$

# to regenerage the list in vim:
# ++d}- :r! find html -type f -exec echo '$(srcdir)/'\{} \\ \;
# }bxx
nobase_dist_doc_DATA = \
$(srcdir)/html/appendix.html \
$(srcdir)/html/index.html \
$(srcdir)/html/getstarted.html \
$(srcdir)/html/extensions.html \
$(srcdir)/html/_static/file.png \
$(srcdir)/html/_static/fityk.ico \
$(srcdir)/html/_static/unipress-button.png \
$(srcdir)/html/_static/fityk-banner.png \
$(srcdir)/html/_static/pygments.css \
$(srcdir)/html/_static/searchtools.js \
$(srcdir)/html/_static/navigation.png \
$(srcdir)/html/_static/contents.png \
$(srcdir)/html/_static/jquery.js \
$(srcdir)/html/_static/plus.png \
$(srcdir)/html/_static/fityk.css \
$(srcdir)/html/_static/minus.png \
$(srcdir)/html/_static/wxwidgets_powered.png \
$(srcdir)/html/_static/basic.css \
$(srcdir)/html/_static/doctools.js \
$(srcdir)/html/_static/sphinxdoc.css \
$(srcdir)/html/intro.html \
$(srcdir)/html/searchindex.js \
$(srcdir)/html/fityk-manual.html \
$(srcdir)/html/_images/fityk076.png \
$(srcdir)/html/_images/project-support.jpg \
$(srcdir)/html/_images/math/9e9c8a0fb6a92d4c3f969ab7a959a80f3069e3c9.png \
$(srcdir)/html/_images/math/5e97a8af68fbc8e357d3ee0eba452022b06c1875.png \
$(srcdir)/html/_images/math/7532b41a34c9ca592694135d1de65eeb8d9ce036.png \
$(srcdir)/html/_images/math/727873d2cbeda3d30f41bee18cb2bf5ea1071fa1.png \
$(srcdir)/html/_images/math/fa35d9fc104207e09a712110ac81612c5b279a6c.png \
$(srcdir)/html/_images/math/61c594c97b61f72af93af4e61894b3f16a3bbb30.png \
$(srcdir)/html/_images/math/ee5c7c921b3334f6ecf58c241eeea42acf0ce71e.png \
$(srcdir)/html/_images/math/572b65ebc3f438b176b4ca4a890799f2f72564af.png \
$(srcdir)/html/_images/math/bcdaf43e7130e8f85d7721440139a9481b23e631.png \
$(srcdir)/html/_images/math/9f4a323840470c45eb00c5896ba579edc0f0799f.png \
$(srcdir)/html/_images/math/d953af3188b3c252a5220e76255e375ee0afaf07.png \
$(srcdir)/html/_images/math/707fc7dbf98d8f4f39c6bdf3f6c025ed948789c9.png \
$(srcdir)/html/_images/math/d3d29774850d1fa81d750fef3ed9b65ee66c524e.png \
$(srcdir)/html/_images/math/ce4588fd900d02afcbd260bc07f54cce49a7dc4a.png \
$(srcdir)/html/_images/math/7508455f7c089e149392451dec57d771370d63ce.png \
$(srcdir)/html/_images/math/19f15eacccc345c3179259320491956f3f23887a.png \
$(srcdir)/html/_images/math/695f416425dbcafe8c762a1e8c533023350a525d.png \
$(srcdir)/html/_images/math/8863e0082a6f43362818a581cde27eaeced7fe3a.png \
$(srcdir)/html/_images/math/d3e7d7cd926970086d34700ac3f979eac398132a.png \
$(srcdir)/html/_images/math/566b01428cbc7f85e5dcac822761913d6fd48795.png \
$(srcdir)/html/_images/math/486938d1819df5972cb80ef2d101517b20412366.png \
$(srcdir)/html/_images/math/c104a975652392e8192a26551dc776a5fbdcf9c7.png \
$(srcdir)/html/_images/math/a61c1e51680e27d35d05bebed87d474972fc70bd.png \
$(srcdir)/html/_images/math/98327fe212a6e72966017a3e21780935b66080f8.png \
$(srcdir)/html/_images/math/1ab39d761413804680d26d972381f028001562f5.png \
$(srcdir)/html/_images/math/441e7ddea6580ae19be0583f35b648340340f789.png \
$(srcdir)/html/_images/math/7567e40901ed423f42f5fc972bca0fd2d311ec40.png \
$(srcdir)/html/_images/math/6e4754d11ef093459abaf1935c69be98b7cf1430.png \
$(srcdir)/html/_images/math/75870e6ce52429cdac61f596a15826f179ae817e.png \
$(srcdir)/html/_images/math/349202f044a9aca8885d8084005ea099f1816b94.png \
$(srcdir)/html/_images/math/9fabdcba50c720ea1ae42cd0b4f1d3eb5f367e56.png \
$(srcdir)/html/_images/math/412787c048e28774dc63fc27db42dc52ca858de7.png \
$(srcdir)/html/_images/math/c2ec895a1a42f04814ac1e370c4b8ca8fb702167.png \
$(srcdir)/html/_images/math/a1f49d0ef1ed40eddc5de7055dc1378db1e4a95c.png \
$(srcdir)/html/_images/math/23decadd8b245289f5fa993f2c99e0ac9cff56dc.png \
$(srcdir)/html/_images/math/db7ec4a5b3fa9a0f1eaefb781da25807d831f2c5.png \
$(srcdir)/html/ref.html \
$(srcdir)/html/search.html \
$(srcdir)/html/_sources/appendix.txt \
$(srcdir)/html/_sources/index.txt \
$(srcdir)/html/_sources/extensions.txt \
$(srcdir)/html/_sources/getstarted.txt \
$(srcdir)/html/_sources/ref.txt \
$(srcdir)/html/_sources/intro.txt \
$(srcdir)/html/_sources/fityk-manual.txt


dist_man_MANS = fityk.1

EXTRA_DIST  = appendix.rst fityk-manual.rst index.rst ref.rst \
	      extensions.rst getstarted.rst intro.rst \
	      layout.html conf.py

SPHINX = sphinx-build -a -d ./doctrees/

all-local: $(srcdir)/html/appendix.html

$(srcdir)/html/appendix.html: $(srcdir)/appendix.rst $(srcdir)/fityk-manual.rst $(srcdir)/index.rst $(srcdir)/ref.rst $(srcdir)/extensions.rst $(srcdir)/getstarted.rst $(srcdir)/intro.rst $(srcdir)/conf.py 
	$(SPHINX) -b html $(srcdir) $(srcdir)/html
	rm -f $(srcdir)/html/objects.inv $(srcdir)/html/.buildinfo
	sed -i -e 's/fityk — free peak fitting software &mdash; Fityk 0.8.9 manual/fityk \&mdash; free peak fitting software/' $(srcdir)/html/index.html

pdf:
	rm -rf $(srcdir)/doctrees/
	$(SPHINX) -b latex $(srcdir) latex
	cd latex/ && make all-pdf

