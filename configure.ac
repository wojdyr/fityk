# $Id$
# Process this file with autoconf to produce a configure script.

AC_INIT([fityk], [0.8.3])
LIBRARY_VERSION_FLAG="-version-info 0:0:0"

AC_CONFIG_SRCDIR([src/voigt.h])
AC_CONFIG_AUX_DIR(config)
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

AC_ARG_ENABLE(python, 
[  --enable-python         build python bindings to libfityk])

AC_ARG_ENABLE(3rdparty, 
[  --disable-3rdparty      do not use 3rdparty directory])
test -z "$enable_3rdparty" && enable_3rdparty="yes"  

# The GNU readline library
AC_ARG_WITH(readline, 
[  --without-readline      do not use the GNU readline library for cfityk])
test -z "$with_readline" && with_readline="yes"  

# don't run make in docs/
AC_ARG_WITH(doc,
[  --without-doc           do not build and install documentation]) 
test -z "$with_doc" && with_doc="yes"  

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
 [Boost::Spirit headers were not found 
  or your compiler can't compile it.
  If you use CVS version, make sure you have Spirit installed (see INSTALL).
 ])])

# see comments above...
CPPFLAGS=$OLD_CPPFLAGS

if test "$enable_3rdparty" = "yes"; then
	CPPFLAGS_3RDPARTY="-I\$(top_srcdir)/3rdparty -I\$(top_srcdir)/3rdparty/xylib" 
else
# xylib is in 3rdparty, but it's always necessary 
	CPPFLAGS_3RDPARTY="-I\$(top_srcdir)/3rdparty/xylib" 
fi

XYLIB_LDADD="\$(top_builddir)/3rdparty/xylib/xylib/libxy.la"


AC_LANG_POP([C++]) dnl C is faster than C++

SRCSUBDIRS=
# Checks for version specific libraries and headers.
if test "$enable_GUI" != "no" ; then 
    SRCSUBDIRS="$SRCSUBDIRS wxgui"
    AM_PATH_WXCONFIG(2.6.3, wxWin=1)
    if test "$wxWin" != 1; then
    	AC_MSG_ERROR([
    	      wxWindows must be installed on your system
    	      but wx-config script couldn't be found.
    
    	      Please check that wx-config is in path, the directory
    	      where wxWindows libraries are installed (returned by
    	      'wx-config --libs' command) is in LD_LIBRARY_PATH or
    	      equivalent variable and wxWindows version is 2.6.3 or above.
            ])
    fi
fi

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
      [You don't have an unistd.h header, that is required to build cfityk.])])

fi

if test "$with_doc" != "no"; then
    DOC=doc
else
    DOC=
fi

if test "$enable_python" = "yes"; then 
    AM_PATH_PYTHON(2.3)
    AC_PYTHON_DEVEL
    AC_PROG_SWIG(1.3.17)
    SWIG="$SWIG -c++"
    AC_SUBST([SWIG_PYTHON_OPT],[-python])
    AC_SUBST([SWIG_PYTHON_CPPFLAGS],[$PYTHON_CPPFLAGS])
    PYBINDINGDIR=python
else
    PYBINDINGDIR=
fi

AC_SUBST(SRCSUBDIRS)
AC_SUBST(DOC)
AC_SUBST(PYBINDINGDIR)
AC_SUBST(CPPFLAGS_3RDPARTY)
AC_SUBST(XYLIB_LDADD)
AC_SUBST(LIBRARY_VERSION_FLAG)

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

AC_DEFINE_UNQUOTED(CONFIGURE_BUILD, "$ac_cv_build", [ac_cv_build])
AC_DEFINE_UNQUOTED(CONFIGURE_ARGS, "$ac_configure_args", [ac_configure_args])

AC_CONFIG_FILES([Makefile fityk.spec fityk.iss src/Makefile src/wxgui/Makefile src/cli/Makefile src/wxgui/img/Makefile doc/Makefile doc/fitykhelp_img/Makefile samples/Makefile config/Makefile 3rdparty/Makefile 3rdparty/xylib/Makefile 3rdparty/xylib/xylib/Makefile python/Makefile])
AC_OUTPUT


