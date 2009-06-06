# $Id: Makefile.am 354 2007-11-19 12:38:52Z wojdyr $

lib_LTLIBRARIES = libxy.la

libxy_la_LDFLAGS = "-version-info 1:0:0"

# This is useful when building as a part of fityk (which provides subset of
# boost library in 3rdparty/ dir). It's not used in standalone xylib build,
# which requires boost library to be installed (just to make it easier).
libxy_la_CPPFLAGS = @CPPFLAGS_3RDPARTY@


libxy_la_SOURCES = xylib.cpp cache.cpp brucker_raw.cpp \
		   gsas.cpp pdcif.cpp philips_raw.cpp \
		   philips_udf.cpp rigaku_dat.cpp text.cpp \
		   uxd.cpp vamas.cpp winspec_spe.cpp cpi.cpp dbws.cpp \
		   canberra_mca.cpp xfit_xdd.cpp riet7.cpp \
                   util.cpp util.h

pkginclude_HEADERS = xylib.h cache.h brucker_raw.h \
  		     gsas.h pdcif.h philips_raw.h philips_udf.h \
		     rigaku_dat.h text.h uxd.h vamas.h winspec_spe.h \
		     cpi.h dbws.h canberra_mca.h xfit_xdd.h riet7.h

