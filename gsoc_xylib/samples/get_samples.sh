#!/bin/bash
# download some sample files, which we do not have re-distribution rights,  from public domain

# diffract v1 raw files
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample2.raw -P diffracat_v1_raw/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample3.raw -P diffracat_v1_raw/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample4.raw -P diffracat_v1_raw/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample5.raw -P diffracat_v1_raw/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample6.raw -P diffracat_v1_raw/

# diffract v2/v3 raw files
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-1a.raw -P diffracat_v2v3_raw/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-1b.raw -P diffracat_v2v3_raw/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-1c.raw -P diffracat_v2v3_raw/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-2.raw -P diffracat_v2v3_raw/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/raw/cpd-3.raw -P diffracat_v2v3_raw/

#  pdCIF format files
	# ic0111177.cif
wget -N http://pubs.acs.org/subscribe/journals/inocaj/suppinfo/41/i14/ic0111177/ic0111177.cif?sessid=8929 -P pdcif/
	#ic034984fsi20030819_115442.cif
wget -N http://pubs.acs.org/subscribe/journals/inocaj/suppinfo/ic034984f/ic034984fsi20030819_115442.cif?sessid=1240 -P pdcif/
wget -N http://159.226.150.7/cifs/2005No7/1829.cif -P pdcif/
wget -N http://www.ccp14.ac.uk/ccp/ccp14/ftp-mirror/briantoby/pub/cryst/cif/NISI.cif -P pdcif/

# philips rd files
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-1a.rd -P philips_rd/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-1b.rd -P philips_rd/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-1c.rd -P philips_rd/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-2.rd -P philips_rd/
wget -N http://www.mx.iucr.org/iucr-top/comm/cpd/QARR/rd/cpd-3.rd -P philips_rd/

# uxd files
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample1.uxd  -P uxd/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample2.uxd  -P uxd/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample3.uxd  -P uxd/
wget -N http://sdpd.univ-lemans.fr/course/week-1/sample4.uxd  -P uxd/
wget -N http://sdpd.univ-lemans.fr/DU-SDPD/solutions/week-3/synchro.uxd -P uxd/

# VAMAS files
wget -N http://www.surfacespectra.com/xps/download/fomblin_y.vms -P vamas/

echo "done!"


