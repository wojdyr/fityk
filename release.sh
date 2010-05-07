#!/bin/bash 
# $Id$

version=0.9.2
WEB="iris.unipress.waw.pl:www/fityk2/"

MINGW_DIR=mingw-build
ALL_WIN_FILES=all_files #inside of $MINGW_DIR

win_setup_filename=$MINGW_DIR/all_files/Output/fityk-$version-setup.exe
tarball_filename=fityk-$version.tar.bz2

if [ $# -eq 0 ]; then 
 echo Version in this script is set to $version
 echo usage: $0 step_nr
 echo steps:
 echo 0. prepare new version and increase version number 
 echo 1. test if everything compiles after distclean, run samples
 echo 2. test if svn can be compiled with other settings
 echo 3. make tarball
 echo 4. compile windows version and make installer
 echo 5. https://build.opensuse.org/project/show?project=home%3Awojdyr
 echo 8. SourceForge release
 echo 9. put docs on www
 echo "10. http://freshmeat.net/projects/fityk/releases/new "
 echo "    http://www.gnomefiles.org/devs/index.php?login    "
 exit
fi

echo
echo        Step $1 of the release procedure...         
echo
echo -n '===>' 



if [ $1 -eq 0 ]; then
 echo now the version in this script is: $version
 echo and configure.ac contains:
 grep AC_INIT configure.ac
 echo and doc/conf.py:
 grep 'version =' doc/conf.py
 echo
 echo svnversion: `svnversion`
 echo Do not forget to update NEWS and doc/index.rst



elif [ $1 -eq 1 ]; then
 echo  testing compilation and instalation after make distclean

 ./autogen.sh
 make distclean
 BUILD_DIR=test1
 rm -rf $BUILD_DIR
 mkdir $BUILD_DIR
 cd $BUILD_DIR
 ../configure --prefix=$HOME/local --enable-python --with-samples
 make 
 make install
 echo run samples...
 cd ../samples
 cfityk nacl01.fit 
 cfityk test_syntax.fit 
 fityk nacl01.fit
 fityk SiC_Zn.fit


elif [ $1 -eq 2 ]; then
 echo  go to builds/ and run tests ...
 
elif [ $1 -eq 3 ]; then
 echo  make tarball
 ./autogen.sh --prefix=$HOME/local
 make dist-bzip2

# TODO: testing mac compilation
 
elif [ $1 -eq 4 ]; then
 echo Building MS Windows version
 WXMSWINSTALL=$HOME/local/mingw32msvc/
 #WXMSWINSTALL=$HOME/local/mingw32/
 #PATH=$PATH:$HOME/local/mingw32/bin
 #
 # wxWidgets where cross-compiled using debian mingw32* packages:
 #      download wxAll
 #      tar xjf ../tarballs/wxWidgets-2.6.2.tar.bz2
 #      cd wxWidgets-2.6.2/
 #      mkdir build-mingw
 #      cd build-mingw/
 #      ../configure --build=i686-pc-linux-gnu --host=i586-mingw32msvc \
 #        --with-msw --disable-threads --disable-shared --disable-unicode \
 #        --enable-optimise --prefix=$WXMSWINSTALL \
 #        --disable-compat26 \
 #        --without-regex --without-expat --without-odbc \
 #        --without-opengl --without-libjpeg --without-libtiff \
 #        --disable-html --disable-htmlhelp --disable-stc --disable-intl \
 #        --disable-protocols --disable-protocol --disable-fs_inet \
 #        --disable-sockets --disable-ipc --disable-apple_ieee \
 #        --disable-backtrace \
 #        --disable-debugreport --disable-dialupman  --disable-tarstream \
 #        --disable-sound --disable-mediactrl --disable-url --disable-variant \
 #        --disable-aui --disable-xrc --disable-docview \
 #        --disable-logdialog --disable-animatectrl --disable-calendar \
 #        --disable-datepick --disable-tipwindow --disable-popupwin \
 #        --disable-splash --disable-tipdlg \
 #        --disable-wizarddlg  --disable-miniframe --disable-joystick \
 #        --disable-gif  --disable-pcx --disable-tga --disable-iff \
 #        --disable-pnm --disable-mdi --disable-richtext
 #      make; make install
 #
 #rm -rf $MINGW_DIR
 mkdir -p $MINGW_DIR
 cd $MINGW_DIR
 #tar xjf ../$tarball_filename
 SRC_DIR=..
 # host: MinGW from .deb: i586-mingw32msvc, built locally: i586-pc-mingw32
 $SRC_DIR/configure --build=x86_64-pc-linux-gnu --host=i586-mingw32msvc \
   CXXFLAGS="-O3" LDFLAGS="-s" --without-readline \
   --enable-static --disable-shared \
   --with-wx-config=$HOME/local/mingw32msvc/bin/wx-config
 make || exit
 mkdir -p $ALL_WIN_FILES/samples $ALL_WIN_FILES/src
 cp fityk.iss $SRC_DIR/fityk.url $SRC_DIR/COPYING $SRC_DIR/TODO \
    $SRC_DIR/NEWS $ALL_WIN_FILES
 cp -r $SRC_DIR/doc/html/ $ALL_WIN_FILES/
 cp $SRC_DIR/samples/*.fit $SRC_DIR/samples/*.dat $SRC_DIR/samples/README \
    $ALL_WIN_FILES/samples/
 cp src/wxgui/fityk.exe src/cli/cfityk.exe $ALL_WIN_FILES/src/

 #echo '"C:\Program Files\HTML Help Workshop\hhc.exe" doc\htmlhelp.hhp' \
 #       				    >$ALL_WIN_FILES/build_help.cmd
 #echo 'ren doc\htmlhelp.chm fitykhelp.chm' >>$ALL_WIN_FILES/build_help.cmd
 echo everything is in: `pwd`/$ALL_WIN_FILES
 

elif [ $1 -eq 8 ]; then
 echo  SF release
 echo uploading files...
 [ ! -e $tarball_filename ] && echo "File not found: $tarball_filename" && exit
 [ ! -e $win_setup_filename ] && echo "File not found: $win_setup_filename" \
                                                                       && exit
 mkdir $version || exit
 cp $tarball_filename $win_setup_filename $version/
 scp -r $version "wojdyr,fityk@frs.sourceforge.net:/home/frs/project/f/fi/fityk/fityk/"
 rm -r $version/
 echo now you may go to:
 echo https://sourceforge.net/project/admin/explorer.php?group_id=79434
 

elif [ $1 -eq 9 ]; then
 echo  putting docs on www   
 echo destination: $WEB
 cd doc/ 
 make all pdf
 echo "sending PDF manual..."
 scp latex/fityk-manual.pdf $WEB/
 echo sending html
 scp -r html/* $WEB/
 echo generating doxygen docs...
 cd ../doxygen/
 doxygen
 echo sending doxygen docs... 
 scp -r html/ $WEB/doxygen/
 cd ..
 

elif [ $1 -eq 10 ]; then
 echo announce: freshmeat.net gnomefiles.com
 # TODO: freshmeat-submit
 
else
 echo unexpected step number: $1
fi

