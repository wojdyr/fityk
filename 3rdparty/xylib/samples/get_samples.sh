#!/bin/bash
# download some sample files, which we do not have re-distribution rights


# Siemens/Bruker diffract v1 raw files
mkdir -p diffracat_v1_raw/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample2.raw -P diffracat_v1_raw/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample3.raw -P diffracat_v1_raw/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample4.raw -P diffracat_v1_raw/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample5.raw -P diffracat_v1_raw/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample6.raw -P diffracat_v1_raw/


# Siemens/Bruker diffract v2/v3 raw files
mkdir -p diffracat_v2v3_raw/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-1a.raw -P diffracat_v2v3_raw/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-1b.raw -P diffracat_v2v3_raw/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-1c.raw -P diffracat_v2v3_raw/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-2.raw -P diffracat_v2v3_raw/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-3.raw -P diffracat_v2v3_raw/


#  pdCIF format files
mkdir -p pdcif/
wget -N http://pubs.acs.org/subscribe/journals/inocaj/suppinfo/41/i14/ic0111177/ic0111177.cif -P pdcif/
wget -N http://pubs.acs.org/subscribe/journals/inocaj/suppinfo/ic034984f/ic034984fsi20030819_115442.cif -P pdcif/
wget -N http://159.226.150.7/cifs/2005No7/1829.cif -P pdcif/
##wget -N http://www.ccp14.ac.uk/ccp/ccp14/ftp-mirror/briantoby/pub/cryst/cif/NISI.cif -P pdcif/
#You can also get some .cif files from the sample directory of ciftools_Linux, 
# an open-source CIF software package which can be download from 
#http://www.ncnr.nist.gov/programs/crystallography/software/cif/ciftools.html


# Philips RD files
mkdir -p philips_rd/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-1a.rd -P philips_rd/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-1b.rd -P philips_rd/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-1c.rd -P philips_rd/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-2.rd -P philips_rd/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-3.rd -P philips_rd/


# Philips UDF files
mkdir -p philips_udf/
# samples found by http://www.google.com/search?q=SampleIdent+filetype%3Audf
# Also, you can get a package of several udf files from 
# http://sdpd.univ-lemans.fr/course/week-1/philips-udf.zip
# (link from: http://sdpd.univ-lemans.fr/course/week-1/sdpd-1.html)
##wget -N http://www.ccp14.ac.uk/ccp/ccp14/ftp-mirror/krumm/Software/windows/winfit/Winfit/ZEOLITE.UDF -P philips_udf/
wget -N http://www.eng.uc.edu/~gbeaucag/Classes/XRD/Labs/2004_Hull_Davey_data/CU1.UDF -P philips_udf/
wget -N http://sdpd.univ-lemans.fr/DU-SDPD/semaine-5/PbCrF1.udf -P philips_udf/


# UXD files
mkdir -p uxd/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample1.uxd  -P uxd/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample2.uxd  -P uxd/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample3.uxd  -P uxd/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample4.uxd  -P uxd/
wget -N http://sdpd.univ-lemans.fr/DU-SDPD/solutions/week-3/synchro.uxd -P uxd/


# Rigaku dat files
mkdir -p rigaku_dat/
# There are Rigaku ".dat" sample files at:
# http://sdpd.univ-lemans.fr/course/week-1/rigaku.zip
# (link from: http://sdpd.univ-lemans.fr/course/week-1/sdpd-1.html)


# VAMAS files
mkdir -p vamas/
wget -N http://www.surfacespectra.com/xps/download/fomblin_y.vms -P vamas/
# data from Michael Richardson <michael.richardson@vuw.ac.nz> 
# (stored at fityk website)
wget -N http://www.unipress.waw.pl/fityk/xylib_samples/mjr9_59c.vms -P vamas/
wget -N http://www.unipress.waw.pl/fityk/xylib_samples/mjr9_64c.vms -P vamas/
wget -N http://www.unipress.waw.pl/fityk/xylib_samples/mjr9_116a.vms -P vamas/


# GSAS files
mkdir -p gsas/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/gss/cpd-1a.gss -P gsas/


# text files
mkdir -p text/
# (stored at fityk website)
wget -N http://www.unipress.waw.pl/fityk/xylib_samples/with_sigma.txt -P text/
wget -N http://www.unipress.waw.pl/fityk/xylib_samples/xy_text.txt -P text/


# Princeton Instruments WinSpec SPE files
mkdir -p winspec_spe/
# sample files from Pablo Bianucci <pbian@physics.utexas.edu> 
# (stored at fityk website)
wget -N http://www.unipress.waw.pl/fityk/xylib_samples/1d-1.spe -P winspec_spe/
wget -N http://www.unipress.waw.pl/fityk/xylib_samples/1d-2.spe -P winspec_spe/
wget -N http://www.unipress.waw.pl/fityk/xylib_samples/1d-3.spe -P winspec_spe/

# Sietronics Sieray CPI files
mkdir -p cpi/
wget -N http://www.chemistry.ohio-state.edu/~woodward/ceo2br.cpi -P cpi/

# DBWS data files
mkdir -p dbws/
wget -N http://mysite.du.edu/~balzar/lebailbr.dbw -P dbws/
wget -N http://sdpd.univ-lemans.fr/DU-SDPD/semaine-4/na5.rit -P dbws/

echo "done!"


