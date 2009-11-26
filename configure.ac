# $Id$
# Process this file with autoconf to produce a configure script.

AC_INIT([fityk], [0.9.0])
#AC_REVISION([$Revision$])
LIBRARY_VERSION_FLAG="-version-info 2:0:0"

AC_CONFIG_SRCDIR([src/voigt.h])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([config])
# AM_CONFIG_HEADER is obsolete, but we are using it for compatibility with
# older versions.
AM_CONFIG_HEADER([config.h])
#AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

# What versions of program are to be made
AC_ARG_ENABLE(GUI,
[  --disable-GUI           do not build GUI version of program])
test -z "$enable_GUI" && enable_GUI="yes"

AC_ARG_ENABLE(CLI,
[  --disable-CLI           do not build CLI version of program])
test -z "$enable_CLI" && enable_CLI="yes"

AC_ARG_ENABLE(xyconvert,
[  --disable-xyconvert     do not build xyconvert program])
test -z "$enable_xyconvert" && enable_xyconvert="yes"

AC_ARG_ENABLE(powdifpat,
[  --enable-powdifpat      build (not working) powdifpat program])
test -z "$enable_powdifpat" && enable_powdifpat="no"

AC_ARG_ENABLE(python,
[  --enable-python         build python bindings to libfityk])

AC_ARG_ENABLE(3rdparty,
[  --disable-3rdparty      do not use 3rdparty directory @<:@default=enable@:>@])
test -z "$enable_3rdparty" && enable_3rdparty="yes"

AC_ARG_WITH(xylib,
[  --with-xylib            use built-in xylib, not standalone one @<:@default=no@:>@])

AC_ARG_WITH(samples,
[  --with-samples          install samples as documentation])

# The GNU readline library
AC_ARG_WITH(readline,
[  --without-readline      do not use the GNU readline library for cfityk])
test -z "$with_readline" && with_readline="yes"

# don't run make in docs/
AC_ARG_WITH(doc,
[  --without-doc           do not build and install documentation])
test -z "$with_doc" && with_doc="yes"

# option added just after CC, CXX and LD. Can be used on Mac for "-isysroot ..."
AC_ARG_WITH(options,
[  --with-options=OPTIONS  options appended to CC, CXX and LD (e.g. -isysroot)],
            [UNIV_OPTION=$withval])


AM_OPTIONS_WXCONFIG

# both CLI and GUI can't be disabled.
if test "$enable_CLI" = "no" -a "$enable_GUI" = "no"; then
	AC_MSG_ERROR([Disabling both GUI and CLI versions makes no sense.])
fi

#################################
# MS Windows specific macros

AC_MSG_CHECKING([for native Win32 - MinGW])
case "${host}" in
    *-*-mingw*)
        os_win32=yes
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


# Checks for programs.
AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

if test "x$UNIV_OPTION" != "x"; then
    CC="$CC $UNIV_OPTION"
    CXX="$CXX $UNIV_OPTION"
    LD="$LD $UNIV_OPTION"
fi

# Checks for common header files.
AC_HEADER_STDC
if test "$ac_cv_header_stdc" != yes; then
	AC_MSG_ERROR([ANSI C header files not found :-(])
fi
# use c++ for testing <sstream>

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(sstream, [], [AC_MSG_ERROR(
 [You don't have a <sstream> header. It is a part of standard C++ library.
  Either you have too old compiler, or standard C++ library can't be found.])])

if test "$with_xylib" != "yes"; then
    AC_CHECK_HEADER([xylib/xylib.h], [], [AC_MSG_ERROR(
 [xylib headers were not found.
  Either use --with-xylib flag or install xylib separately.
 ])])
fi

# use headers from 3rdparty/
# CPPFLAGS="$CPPFLAGS -I\$(top_srcdir)/3rdparty" #doesn't work here(?),
# or substitutes top_srcdir only in Makefile's(?)
OLD_CPPFLAGS=$CPPFLAGS
# can it cause problems?
# CPPFLAGS is set here only for this test, and it is changed later
if test "$enable_3rdparty" = "yes"; then
	CPPFLAGS="$CPPFLAGS -I$srcdir/3rdparty"
fi

AC_CHECK_HEADER([boost/spirit/core.hpp], [], [AC_MSG_ERROR(
 [Boost::Spirit headers were not found or your compiler can not compile it.
  If you use source from svn, make sure you have Spirit installed (see INSTALL).
 ])])

# see comments above...
CPPFLAGS=$OLD_CPPFLAGS



if test "$enable_3rdparty" = "yes"; then
	CPPFLAGS_3RDPARTY="-I\$(top_srcdir)/3rdparty"
else
	CPPFLAGS_3RDPARTY=
fi

if test "$with_xylib" = "yes"; then
	XYLIB_CPPFLAGS="-I\$(top_srcdir)/xylib"
	XYLIB_LDADD="\$(top_builddir)/xylib/xylib/libxy.la"
else
	XYLIB_CPPFLAGS=
        XYLIB_LDADD="-lxy"
fi
AM_CONDITIONAL([XYLIB_DIR], [test "$with_xylib" = yes])

AM_CONDITIONAL([SAMPLES_DIR], [test "$with_samples" = yes])

AC_LANG_POP([C++]) dnl C is faster than C++

SRCSUBDIRS=
# Checks for version specific libraries and headers.
if test "$enable_GUI" != "no" ; then
    SRCSUBDIRS="$SRCSUBDIRS wxgui"
    AM_PATH_WXCONFIG([2.8.4], [wxWin=1], [wxWin=0], [adv,core,base])
    if test "$wxWin" != 1; then
    	AC_MSG_ERROR([
    	      wxWindows must be installed on your system
    	      but wx-config script couldn't be found.

    	      Please check that wx-config is in path, the directory
    	      where wxWindows libraries are installed (returned by
    	      'wx-config --libs' command) is in LD_LIBRARY_PATH or
    	      equivalent variable and wxWindows version is 2.8.4 or above.
            ])
    fi
fi

if test "$enable_xyconvert" != "no" ; then
    WX_CONFIG_CHECK([2.9.0], [], [AC_MSG_ERROR([
	 xyConvert requires wxWidgets >= 2.9.
	 Either upgrade or add --disable-xyconvert flag.])
		    ])
fi
AM_CONDITIONAL([XYCONVERT], [test $enable_xyconvert != no])

AM_CONDITIONAL([POWDIFPAT], [test $enable_powdifpat != no])


if test "$enable_CLI" != "no"; then
    SRCSUBDIRS="$SRCSUBDIRS cli"
    READLINE_STUFF

    AC_CHECK_PROG(gnuplot_in_path, gnuplot, yes)
    if test "$gnuplot_in_path" != "yes" ; then
	    AC_MSG_RESULT(
	  [ gnuplot is used by cfityk (command line version) for drawing plots,
		       but it is not necessery])
    fi

    AC_CHECK_HEADER(unistd.h, [], [AC_MSG_ERROR(
      [You do not have an unistd.h header, that is required to build cfityk.])])

fi

AM_CONDITIONAL([DOC_DIR], [test "$with_doc" != no])

if test "$enable_python" = "yes"; then
    AM_PATH_PYTHON(2.3)
    AC_PYTHON_DEVEL
    AC_PROG_SWIG(1.3.17)
fi
AM_CONDITIONAL([SWIG_DIR], [test "$enable_python" = yes])

AC_SUBST(SRCSUBDIRS)
AC_SUBST(CPPFLAGS_3RDPARTY)
AC_SUBST(XYLIB_CPPFLAGS)
AC_SUBST(XYLIB_LDADD)
AC_SUBST(LIBRARY_VERSION_FLAG)

# define XYLIB_ADDLIB for compatibility with xylib/configure.ac
XYLIB_ADDLIB=
AC_SUBST(XYLIB_ADDLIB)

# before checking erf, see if a basic math function (and linking) works
AC_CHECK_LIB([m], [sin], [], [])
# check for erf and erfc function
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

AC_CHECK_FUNCS([popen getline])

AC_DEFINE_UNQUOTED(CONFIGURE_BUILD, "$ac_cv_build", [ac_cv_build])
AC_DEFINE_UNQUOTED(CONFIGURE_ARGS, "$ac_configure_args", [ac_configure_args])

AC_CONFIG_FILES([Makefile fityk.iss
		 src/Makefile
		 src/wxgui/Makefile
		 src/cli/Makefile
		 src/wxgui/img/Makefile
		 doc/Makefile
		 samples/Makefile
		 xylib/Makefile
		 xylib/xylib/Makefile
		 python/Makefile])
AC_OUTPUT

