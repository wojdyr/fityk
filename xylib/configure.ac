# $Id$
# after changing, run "autoreconf --install --verbose"

AC_PREREQ(2.59)
AC_INIT([xylib], [0.5], [])
AC_CONFIG_SRCDIR([xylib/xylib.cpp])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([1.9 dist-bzip2 foreign])

# optional zlib and bzlib libraries
AC_ARG_WITH(zlib,
 [  --without-zlib                disable zlib support (reading .gz files)])
test -z "$with_zlib" && with_zlib="yes"

AC_ARG_WITH(bzlib,
 [  --without-bzlib               disable bzlib support (reading .bz2 files)])
test -z "$with_bzlib" && with_bzlib="yes"

# Checks for programs.
AC_PROG_CXX
AC_PROG_LIBTOOL

# Checks for libraries.
XYLIB_ADDLIB=
if test "$with_zlib" = "yes"; then
  AC_CHECK_LIB([z], [gzopen],
               [AC_DEFINE([HAVE_ZLIB], [1], [Define if zlib is available])],
               [AC_MSG_ERROR(
  [zlib library was not found.
   Either use flag --without-zlib or install the library.
 ])])
  XYLIB_ADDLIB="$XYLIB_ADDLIB -lz"
  AC_CHECK_HEADER([zlib.h], [], [AC_MSG_ERROR([
   zlib.h header was not found. Either use flag --without-zlib or install
   zlib library (including header files).
 ])])
fi

if test "$with_bzlib" = "yes"; then
  AC_CHECK_LIB([bz2], [BZ2_bzReadOpen],
               [AC_DEFINE([HAVE_BZLIB], [1], [Define if bzlib is available])],
               [AC_MSG_ERROR(
  [bzlib library was not found.
   Either use flag --without-bzlib or install the library.
 ])])
  XYLIB_ADDLIB="$XYLIB_ADDLIB -lbz2"
  AC_CHECK_HEADER([bzlib.h], [], [AC_MSG_ERROR([
   bzlib.h header was not found. Either use flag --without-bzlib or install
   bzlib library (including header files).
 ])])
fi

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
AC_CHECK_HEADERS([sys/types.h sys/stat.h], [],
                 [AC_MSG_ERROR([Headers containg POSIX stat() are not found
		  Please inform xylib maintainer about this problem,
		  including information about your compiler. ])])
AC_CHECK_HEADERS([tr1/memory],
                 [AC_DEFINE([XYLIB_HAVE_TR1_MEMORY], [1],
	                            [Define if <tr1/memory> is available])])

# We change directly the header xylib/cache.h, because including <config.h>
# in this header would cause problems in programs using xylib.
uses_tr1_memory=`grep '#define XYLIB_USE_TR1_MEMORY ' $srcdir/xylib/cache.h | sed 's/.*\([01]\)/\1/g'`
if test "$ac_cv_header_tr1_memory" = "yes" && test $uses_tr1_memory != "1"; then
    sed -i 's/\(#define XYLIB_USE_TR1_MEMORY\) 0/\1 1/' $srcdir/xylib/cache.h
elif test "$ac_cv_header_tr1_memory" != "yes" && test $uses_tr1_memory = "1"; then
    sed -i 's/\(#define XYLIB_USE_TR1_MEMORY\) 1/\1 0/' $srcdir/xylib/cache.h
fi

# this is for compatibility with fityk configure.ac 
# see xylib/Makefile.am for comments on CPPFLAGS_3RDPARTY
AC_SUBST(CPPFLAGS_3RDPARTY)

AC_SUBST(XYLIB_ADDLIB)

AC_OUTPUT([
	   Makefile
	   xylib/Makefile
])

