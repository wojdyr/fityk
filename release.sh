#!/bin/bash 

version=1.3.0
WEB="iris.unipress.waw.pl:www/fityk2/"

MINGW_DIR=mingw-build

win_setup_filename=$MINGW_DIR/all_files/Output/fityk-$version-setup.exe
tarball_filename=fityk-$version.tar.bz2

if [ $# -eq 0 ]; then 
 echo Version in this script is set to $version
 echo usage: $0 step_nr
 echo steps:
 echo a. update autotools files and m4 scripts from autoconf-archive
 echo 0. prepare new version and increase version number 
 echo 1. run samples
 echo "2. cd doc && make && make pdf"
 echo 3. make tarball: make dist-bzip2
 echo 4. compile windows version and make installer
 echo 5. https://build.opensuse.org/project/show?project=home%3Awojdyr
 echo 8. SourceForge release
 echo 9. put docs on www
 echo "10. http://freshmeat.net/projects/fityk/releases/new "
 echo "    http://www.gnomefiles.org/devs/index.php?login    "
 echo "12. git tag -a v$version -m 'version $version'; git push --tags"
 exit
fi

echo
echo        Step $1 of the release procedure...         
echo
echo -n '===>' 


if [ $1 = "a" ]; then
 echo update m4 scripts from autoconf-archive...
 ac_scripts="ax_lua.m4 ax_python_devel.m4 ax_lib_readline.m4 ax_pkg_swig.m4"
 for i in $ac_scripts; do
     curl "http://git.savannah.gnu.org/gitweb/?p=autoconf-archive.git;a=blob_plain;f=m4/$i" -o m4/$i
 done
 rm -r m4/libtool.m4 m4/lt* build-aux/* aclocal.m4
 autoreconf -ifv


elif [ $1 -eq 0 ]; then
 echo now the version in this script is: $version
 echo configure.ac:
 grep AC_INIT configure.ac
 echo doc/conf.py:
 grep 'version =' doc/conf.py
 echo doc/index.rst:
 grep 'Version ' doc/index.rst
 echo NEWS:
 head -4 NEWS | grep version
 echo

elif [ $1 -eq 1 ]; then
 echo run tests and samples...
 make check || exit
 cd ./tests
 ../cli/cfityk test_syntax.fit || exit
 cd ../samples
 ./hello
 ../cli/cfityk nacl01.fit
 ../wxgui/fityk nacl01.fit
 ../wxgui/fityk SiC_Zn.fit


elif [ $1 -eq 2 ]; then
 cd doc && make && make pdf
 
elif [ $1 -eq 3 ]; then
 echo  make tarball
 make dist-bzip2
 #cd builds/
 #make daily

elif [ $1 -eq 4 ]; then
 echo Building MS Windows version in $MINGW_DIR
 unset GREP_OPTIONS # http://debbugs.gnu.org/cgi/bugreport.cgi?bug=16259
 set -e
 MDIR=$HOME/local/mingw32
 BOOST_DIR=$HOME/local/src/boost_1_50_0
 [ -d "$MINGW_DIR" ] || mkdir -p $MINGW_DIR
 cd "$MINGW_DIR"
 if ! [ -e Makefile ]; then
     ../configure --host=i686-w64-mingw32 \
       CPPFLAGS="-I$BOOST_DIR -I$MDIR/include/" \
       CFLAGS="-O3" CXXFLAGS="-O3" \
       LDFLAGS="-s -fno-keep-inline-dllexport -L$MDIR/lib" \
       LUA=$MDIR/bin/lua.exe \
       --without-readline --with-wx-config=$MDIR/bin/wx-config \
       --disable-static --enable-shared --enable-nlopt
 fi
 make -j2 || exit
 outdir=all_files
 mkdir -p $outdir/samples $outdir/fityk
 cp fityk.iss ../fityk.url ../COPYING ../TODO ../NEWS $outdir/
 cp -r ../doc/html/ $outdir/
 cp ../samples/*.fit ../samples/*.dat ../samples/*.lua \
    ../samples/README $outdir/samples/
 cp wxgui/.libs/*.exe cli/.libs/cfityk.exe \
    fityk/.libs/libfityk-*.dll \
    $MDIR/bin/libxy-*.dll $MDIR/bin/xyconv.exe $MDIR/bin/xyconvert.exe \
    $MDIR/bin/lua5*.dll \
    $MDIR/bin/libnlopt*.dll \
    $MDIR/lib/wxbase30u_gcc_custom.dll \
    $MDIR/lib/wxmsw30u_adv_gcc_custom.dll \
    $MDIR/lib/wxmsw30u_core_gcc_custom.dll \
    $MDIR/lib/wxmsw30u_stc_gcc_custom.dll \
    $outdir/fityk/
 cp /usr/i686-w64-mingw32/sys-root/mingw/bin/libgcc_s_sjlj-1.dll \
    /usr/i686-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll \
    /usr/i686-w64-mingw32/sys-root/mingw/bin/zlib1.dll \
    /usr/i686-w64-mingw32/sys-root/mingw/bin/libpng16-16.dll \
    $outdir/fityk/
 i686-w64-mingw32-strip $outdir/fityk/*
 echo everything is in: `pwd`/$outdir
 

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
 echo or to http://sourceforge.net/projects/fityk/files/
 

elif [ $1 -eq 9 ]; then
 #echo  putting docs on www
 #echo destination: $WEB
 cd doc/ 
 make all pdf
 echo "now upload PDF manual..."
 #echo "sending PDF manual..."
 #scp latex/fityk-manual.pdf $WEB/
 #echo sending html
 #scp -r html/* $WEB/
 #echo generating doxygen docs...
 #cd ../doxygen/
 #doxygen
 #echo sending doxygen docs...
 #scp -r html/ $WEB/doxygen/
 #cd ..
 

elif [ $1 -eq 10 ]; then
 echo announce: freshmeat.net gnomefiles.com
 
else
 echo unexpected step number: $1
fi

