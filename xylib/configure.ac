# $Id$
# after changing, run "autoreconf --install --verbose"

AC_PREREQ(2.59)
AC_INIT([xylib], [0.4], [])
AC_CONFIG_SRCDIR([xylib/xylib.cpp])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([1.9 dist-bzip2 foreign])

# Checks for programs.
AC_PROG_CXX
AC_PROG_LIBTOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for header files.
AC_LANG([C++])
AC_CHECK_HEADERS([cstdlib cstring climits vector], [],
                 [AC_MSG_ERROR([Could not find necessary C++ libs headers])])
AC_CHECK_HEADERS([boost/cstdint.hpp boost/detail/endian.hpp], [],
                 [AC_MSG_ERROR([Could not find necessary Boost headers])])
AC_CHECK_HEADERS([boost/spirit/core.hpp], [],
                 [AC_MSG_ERROR([Boost::Spirit headers were not found.])])

#AC_CHECK_HEADERS([tr1/memory], [],
#                 [AC_MSG_ERROR([<tr1/memory> headers were not found.
#		  Please inform xylib maintainer about this problem,
#		  including information about your compiler. ])])

AC_CHECK_HEADERS([sys/types.h sys/stat.h], [],
                 [AC_MSG_ERROR([Headers containg POSIX stat() are not found
		  Please inform xylib maintainer about this problem,
		  including information about your compiler. ])])

# this is for compatibility with fityk configure.ac 
# see xylib/Makefile.am for comments on CPPFLAGS_3RDPARTY
AC_SUBST(CPPFLAGS_3RDPARTY)

AC_OUTPUT([
	   Makefile
	   xylib/Makefile
])

