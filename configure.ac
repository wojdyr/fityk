# Process this file with autoconf to produce a configure script.

AC_INIT([fityk],[1.3.2])
LIBRARY_VERSION_FLAG="-version-info 4:0:0"

AC_CONFIG_SRCDIR([fityk/voigt.h])
AC_CONFIG_AUX_DIR(build-aux)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([1.11 foreign subdir-objects silent-rules -Wall])

AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT([disable-static win32-dll])


AC_ARG_ENABLE(GUI, AS_HELP_STRING([--disable-GUI], [do not build the GUI]))
AC_ARG_ENABLE(CLI, AS_HELP_STRING([--disable-CLI], [do not build the CLI]))
AC_ARG_ENABLE(powdifpat, AS_HELP_STRING([--enable-powdifpat],
                              [build (not working) powdifpat program]))
AC_ARG_ENABLE(python, AS_HELP_STRING([--enable-python],
                              [build Python bindings to libfityk]))
#AC_ARG_ENABLE(perl, AS_HELP_STRING([--enable-perl],
#                              [build Perl bindings to libfityk]))
AC_ARG_ENABLE(nlopt, AS_HELP_STRING([--enable-nlopt],
                              [use NLopt library]))

# this option is handled in m4/readline.m4
AC_ARG_WITH(readline, AS_HELP_STRING([--without-readline],
                       [do not use the GNU readline library for cfityk]))

AM_OPTIONS_WXCONFIG


AC_MSG_CHECKING([for MinGW compiler])
case "$host_os" in
    mingw*)
        windows=yes
	;;
    *)
        windows=no
	;;
esac
AC_MSG_RESULT([$windows])
AM_CONDITIONAL(OS_WIN32, test "$windows" = "yes")
if test "$windows" = "yes"; then
    AC_CHECK_TOOL(RC, windres)
    if test "x$RC" = "x"; then
        AC_MSG_ERROR([Required resource tool 'windres' not found on PATH.])
    fi
fi


if test "x$with_options" != "x"; then
    CC="$CC $with_options"
    CXX="$CXX $with_options"
    LD="$LD $with_options"
fi

AC_LANG_PUSH([C++])

# AC_PROG_CXX doesn't fail if compiler is not found, test it explicitely
AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])], [],
               [AC_MSG_ERROR([C++ compiler not found.])])

AC_CHECK_HEADER(sstream, [], [AC_MSG_ERROR(
 [You don't have a <sstream> header. It is a part of standard C++ library.
  Either you have too old compiler, or standard C++ library can't be found.])])

AC_CHECK_HEADER([boost/math/special_functions/gamma.hpp], [], [AC_MSG_ERROR(
 [Boost::Math headers not found. Make sure you have Boost installed.])])

AC_LANG_POP([C++])

AC_CHECK_HEADER([zlib.h], [], [AC_MSG_ERROR(
 [zlib.h header not found. Install zlib library (with development files).])])

AC_CHECK_FUNCS([popen getline])
AC_SEARCH_LIBS([cos], [m], [], [
                AC_MSG_ERROR([unable to find the dlopen() function])])
AC_CHECK_FUNC(erf, [], [AC_MSG_ERROR([erf function not found (?).
                  Please inform program developer(s) about this problem.])])
AC_CHECK_FUNC(erfc, [], [AC_MSG_ERROR([erfc function not found (?).
                  Please inform program developer(s) about this problem.])])
AC_CHECK_FUNCS([finite isnan])
# see m4/ax_cxx_have_isfinite.m4
AX_CXX_HAVE_ISFINITE

AC_CHECK_HEADER([xylib/xylib.h], [], [AC_MSG_ERROR(
 [xylib headers were not found. You may need to install xylib first.])])

AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([@%:@include <xylib/xylib.h>], [
#if XYLIB_VERSION < 10600
# error "xylib >= 1.6 is required"
#endif
  ])],
  [], [AC_MSG_ERROR([xylib >= 1.6 is not installed])])

AS_IF([test x"$LUA52_FROM_SOURCE" = xyes], [
    LUA_INCLUDE='-I$(top_srcdir)/fityk/lua52/src'
  ], [
    AX_PROG_LUA(5.1, 5.5)
    AX_LUA_HEADERS
    AX_LUA_LIBS
])

# Checks for version specific libraries and headers.
AS_IF([test "x$enable_GUI" != "xno"], [
    AM_PATH_WXCONFIG([3.0.0], [wxWin=1], [wxWin=0], [adv,core,base,stc])
    if test "$wxWin" != 1; then
    	AC_MSG_ERROR([
	      wxWidgets must be installed on your system
	      but wx-config script could not be found.

	      Please check that wx-config is in path, the directory
	      where wxWidgets libraries are installed (returned by
	      'wx-config --libs' command) is in LD_LIBRARY_PATH or
	      equivalent variable and wxWidgets version is 3.0 or above.
	     ])
    fi
])


AS_IF([test "x$enable_CLI" != "xno"], [
    if test "x$with_readline" != "xno" ; then
        ORIG_LIBS="$LIBS"
        AX_LIB_READLINE
        LIBS=$ORIG_LIBS
        READLINE_LIBS="$ax_cv_lib_readline"
        # readline < 4.2 doesn't have rl_completion_matches() function
        # some libreadline-compatibile libraries (like libedit) may also
        # not have it. We don't support them.
        AC_CHECK_DECLS([rl_completion_matches], [], [AC_MSG_ERROR([
        Readline library that has rl_completion_matches() not found.
        Either install libreadline >= 4.2 or compatible library,
        or configure fityk with option --without-readline])],
        [
         #include <stdio.h>
         #if defined(HAVE_READLINE_READLINE_H)
         #include <readline/readline.h>
         #elif defined(HAVE_READLINE_H)
         #include <readline.h>
         #endif
        ])
    fi

    AC_CHECK_PROG(gnuplot_in_path, gnuplot, yes)
    if test "x$gnuplot_in_path" != "xyes" ; then
	    AC_MSG_RESULT([gnuplot not found (it is not necessary)])
    fi

    AC_CHECK_HEADER(unistd.h, [], [AC_MSG_ERROR(
                     [unistd.h header not found (required for cfityk.])])
])

if test "x$enable_python" = "xyes"; then
    AM_PATH_PYTHON(2.3)
    AX_PYTHON_DEVEL
fi

if test "x$enable_nlopt" = "xyes"; then
    AC_CHECK_LIB(nlopt, nlopt_version, ,
                 AC_MSG_ERROR([nlopt library not found.]))
    AC_CHECK_HEADER(nlopt.h, , AC_MSG_ERROR([nlopt.h header not found.]))
fi

AM_CONDITIONAL([POWDIFPAT], [test x$enable_powdifpat = xyes])
AM_CONDITIONAL([GUI_DIR], [test "x$enable_GUI" != xno])
AM_CONDITIONAL([CLI_DIR], [test "x$enable_CLI" != xno])
AM_CONDITIONAL([PYTHON_ENABLED], [test "x$enable_python" = xyes])
#AM_CONDITIONAL([PERL_ENABLED], [test "x$enable_perl" = xyes])
AM_CONDITIONAL([NLOPT_ENABLED], [test "x$enable_nlopt" = xyes])
AM_CONDITIONAL([COMPILE_LUA_SRC], [test "x$LUA52_FROM_SOURCE" = xyes])

#if test "x$enable_perl" = "xyes"; then
#    AC_CHECK_PROG(PERL, perl, "yes")
#fi

# 2.0.12 is required here b/c 2.0.11 has a critical bug in Lua generator
AS_IF([test -z "$SWIG"],
      [AX_PKG_SWIG(2.0.12, [], [SWIG=swig_not_found])],
      [AC_MSG_NOTICE([Using SWIG=$SWIG without checking])])
AC_ARG_VAR([SWIG], [the SWIG program])

AC_SUBST(LIBRARY_VERSION_FLAG)
AC_SUBST(LUA_INCLUDE)
AC_SUBST(LUA_LIB)
AC_SUBST(READLINE_LIBS)

AC_DEFINE_UNQUOTED(CONFIGURE_BUILD, "$ac_cv_build", [ac_cv_build])
AC_DEFINE_UNQUOTED(CONFIGURE_ARGS, "$ac_configure_args", [ac_configure_args])

AC_OUTPUT([Makefile
           fityk.iss
           fityk/Makefile
           wxgui/Makefile])

