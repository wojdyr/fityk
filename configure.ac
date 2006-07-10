dnl $Id$
dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([src/voigt.h])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE(fityk, 0.7.5)
AM_MAINTAINER_MODE

dnl What versions of program are to be made 
AC_ARG_ENABLE(GUI, 
[  --disable-GUI           do not build GUI version of program])
test -z "$enable_GUI" && enable_GUI="yes"  
AC_ARG_ENABLE(CLI, 
[  --disable-CLI           do not build CLI version of program])
test -z "$enable_CLI" && enable_CLI="yes"  

dnl normal/debug/optimize flags
AC_ARG_ENABLE(debug, 
[  --enable-debug          debug flags, no optimization       ])
AC_ARG_ENABLE(optimization, 
[  --enable-optimization   all optimizations, no debug info])

dnl Checks for -enable-debug/optimization options
if test "$enable_debug" = "yes"; then 
	CXXFLAGS="-g -O0 -fno-inline -W $CXXFLAGS"
fi
if test "$enable_optimization" = "yes"; then 
	CPPFLAGS="-DNDEBUG $CPPFLAGS"
	CXXFLAGS="-O3 $CXXFLAGS"
	LDFLAGS="-s $LDFLAGS"
fi


dnl The GNU readline library
AC_ARG_WITH(readline, 
[  --without-readline      do not use the GNU readline library for cfityk 
  --with-readline=PATH    Specify the location of readline])
test -z "$with_readline" && with_readline="yes"  

dnl don't run make in docs/
AC_ARG_WITH(doc,
	    [  --without-doc           do not build and install documentation]) 
test -z "$with_doc" && with_doc="yes"  

AM_OPTIONS_WXCONFIG

dnl both CLI and GUI can't be disabled.
if test "$enable_CLI" = "no" -a "$enable_GUI" = "no"; then 
	AC_MSG_ERROR([Disabling both GUI and CLI versions makes no sense.])
fi


PROGRAM_EXT=
#################################
# MS Windows specific macros 

AC_MSG_CHECKING([for native Win32 - MinGW])
case "${host}" in
    *-*-mingw*)
        os_win32=yes
	PROGRAM_EXT=".exe"
	;;
    *)
        os_win32=no
	;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")

if test "$os_win32" = "yes"; then
    AC_CHECK_TOOL(RC, windres)
    if test "${RC}" = "" ; then
        AC_MSG_ERROR([Required resource tool 'windres' not found on PATH.])
    fi
fi
#################################


dnl Checks for programs.
AC_PROG_CXX

dnl Checks for common header files.
AC_HEADER_STDC
if test "$ac_cv_header_stdc" != yes; then 
	AC_MSG_ERROR([ANSI C header files not found :-(])
fi
dnl use c++ for testing <sstream>
 
AC_LANG_PUSH([C++])
AC_CHECK_HEADER(sstream, [], [AC_MSG_ERROR(
 [You don't have a <sstream> header. It is a part of standard C++ library.
  Either you have too old compiler, or standard C++ library can't be found.])])

dnl use headers from 3rdparty/  
dnl CPPFLAGS="$CPPFLAGS -I\$(top_srcdir)/3rdparty" #doesn't work here(?), 
dnl or substitutes top_srcdir only in Makefile's(?)
OLD_CPPFLAGS=$CPPFLAGS
dnl it's an ugly hack, that can cause problems...
dnl CPPFLAGS is set here only for this test, and it is changed later 
CPPFLAGS="$CPPFLAGS -I$srcdir/3rdparty"

AC_CHECK_HEADER([boost/spirit/core.hpp], [], [AC_MSG_ERROR(
 [Boost::Spirit headers were not found 
  or your compiler can't compile it.
  If you use CVS version, make sure you have Spirit installed (see INSTALL).
 ])])

dnl see comments above...
CPPFLAGS="$OLD_CPPFLAGS -I\$(top_srcdir)/3rdparty"

AC_LANG_POP([C++]) dnl C is faster than C++

dnl Checks for version specific libraries and headers.
if test "$enable_GUI" != "no" ; then 
    FITYK=fityk$PROGRAM_EXT 
    AM_PATH_WXCONFIG(2.6.1, wxWin=1)
    if test "$wxWin" != 1; then
    	AC_MSG_ERROR([
    	      wxWindows must be installed on your system
    	      but wx-config script couldn't be found.
    
    	      Please check that wx-config is in path, the directory
    	      where wxWindows libraries are installed (returned by
    	      'wx-config --libs' command) is in LD_LIBRARY_PATH or
    	      equivalent variable and wxWindows version is 2.5.3 or above.
            ])
    fi
fi

if test "$enable_CLI" != "no"; then 
    CFITYK=cfityk$PROGRAM_EXT 
    READLINE_STUFF

    AC_CHECK_PROG(gnuplot_in_path, gnuplot, yes)
    if test "$gnuplot_in_path" != "yes" ; then
	    AC_MSG_RESULT(
	  [ gnuplot is used by cfityk (command line version) for drawing plots, 
		       but it is not necessery])
    fi

    AC_CHECK_HEADER(unistd.h, [], [AC_MSG_ERROR(
      [You don't have an unistd.h header, that is required to build cfityk.])])

fi

if test "$with_doc" != "no"; then
    DOC=doc
else
    DOC=
fi

AC_SUBST(FITYK)
AC_SUBST(CFITYK)
AC_SUBST(DOC)

dnl check for erf and erfc function
AC_CHECK_LIB([m], [erf], [], [AC_MSG_ERROR(
      [You don't have an erf function(?). 
       Please inform program developer(s) about this problem.])])
AC_CHECK_LIB([m], [erfc], [], [AC_MSG_ERROR(
      [You don't have an erfc function(?). 
       Please inform program developer(s) about this problem.])])

AC_CHECK_LIB([m], [finite], [AC_DEFINE([HAVE_FINITE], [1], 
	                            [Define if finite function is available])])
AC_CHECK_LIB([m], [isnan], [AC_DEFINE([HAVE_ISNAN], [1], 
	                            [Define if isnan function is available])])

dnl in generated configure script, this line is too late
test "x$prefix" = xNONE && prefix=$ac_default_prefix

AC_CONFIG_FILES([Makefile fityk.spec fityk.iss src/Makefile src/img/Makefile doc/Makefile doc/fitykhelp_img/Makefile samples/Makefile config/Makefile 3rdparty/Makefile])
AC_OUTPUT


