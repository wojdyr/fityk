# $Id: Makefile.am 354 2007-11-19 12:38:52Z wojdyr $

lib_LTLIBRARIES = libxy.la 

libxy_la_LDFLAGS = "-version-info 0:0:0"
libxy_la_CPPFLAGS = @CPPFLAGS_3RDPARTY@ 


libxy_la_SOURCES = xylib.cpp ds_brucker_raw_v1.cpp ds_brucker_raw_v23.cpp \
		   ds_gsas.cpp ds_pdcif.cpp ds_philips_raw.cpp \
		   ds_philips_udf.cpp ds_rigaku_dat.cpp ds_text.cpp \
		   ds_uxd.cpp ds_vamas.cpp ds_winspec_spe.cpp util.cpp \
		   util.h

include_HEADERS = xylib.h ds_brucker_raw_v1.h ds_brucker_raw_v23.h \
		  ds_gsas.h ds_pdcif.h ds_philips_raw.h ds_philips_udf.h \
		  ds_rigaku_dat.h ds_text.h ds_uxd.h ds_vamas.h \
		  ds_winspec_spe.h

