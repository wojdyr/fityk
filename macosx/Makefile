# This makefile assembles fityk.app from its constiutent parts.  Run "make" to
# build the application bundle or "make dmg" to create a disk image for
# distribution.
#
# Fityk must be compiled first.  See README.txt for more information.


fityk      := ../src/wxgui/fityk
fityk_help := ../doc/fitykhelp.htb
version := $(shell sed -n 's/^AC_INIT(.*, \[\(.*\)\])$$/\1/p' ../configure.ac)


fityk.app: $(fityk) $(fityk_help) Info.plist
	-mkdir fityk.app
	-mkdir fityk.app/Contents
	-mkdir fityk.app/Contents/MacOS
	-mkdir fityk.app/Contents/Resources
	echo -n 'APPL????' > fityk.app/Contents/PkgInfo
	cp Info.plist fityk.app/Contents
	cp $(fityk) fityk.app/Contents/MacOS/fityk
	strip fityk.app/Contents/MacOS/fityk 
	cp fityk.icns fityk.app/Contents/Resources
	cp $(fityk_help) fityk.app/Contents/Resources

../src/wxgui/fityk:
	@echo
	@echo "***   You must compile fityk first   ***"
	@echo
	@exit 1

Info.plist:
	sed -e 's/=V/$(version)/g' < Info.plist.in > Info.plist

dmg: fityk.app
	hdiutil create -srcfolder fityk.app fityk.dmg

clean:
	rm -rf Info.plist fityk.app fityk.dmg

distclean: clean
	@

.PHONY: dmg clean distclean
