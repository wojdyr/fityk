#!/bin/sh

# I'm using automake 1.9.6 and autoconf 2.59

set -x

## first way 
aclocal -I config #-I /usr/local/share/aclocal
autoheader
automake --add-missing --copy
autoconf

# second way -- doesn't work 
#autoreconf -I config -i -v

