# This makefile assembles Fityk.app from its constiutent parts.  Run "make" to
# build the application bundle or "make dmg" to create a disk image for
# distribution.
#
# Fityk must be compiled first.  See README.txt for more information.


fityk      := ../src/wxgui/fityk
fityk_help := ../doc/fitykhelp.htb
version := $(shell sed -n 's/^AC_INIT(.*, \[\(.*\)\])$$/\1/p' ../configure.ac)


Fityk.app: $(fityk) $(fityk_help) Info.plist
	-mkdir Fityk.app
	-mkdir Fityk.app/Contents
	-mkdir Fityk.app/Contents/MacOS
	-mkdir Fityk.app/Contents/Resources
	echo -n 'APPL????' > Fityk.app/Contents/PkgInfo
	cp Info.plist Fityk.app/Contents
	cp $(fityk) Fityk.app/Contents/MacOS/Fityk
	strip Fityk.app/Contents/MacOS/Fityk 
	cp fityk.icns Fityk.app/Contents/Resources
	cp $(fityk_help) Fityk.app/Contents/Resources

../src/wxgui/fityk:
	@echo
	@echo "***   You must compile fityk first   ***"
	@echo
	@exit 1

Info.plist:
	sed -e 's/=V/$(version)/g' < Info.plist.in > Info.plist

dmg: Fityk.app
	hdiutil create -srcfolder Fityk.app Fityk.dmg

clean:
	rm -rf Info.plist Fityk.app Fityk.dmg

distclean: clean
	@

.PHONY: dmg clean distclean
