#!/bin/sh

set -x

## first way 
aclocal -I config #-I /usr/local/share/aclocal
autoheader
automake --add-missing --copy
autoconf

# second way -- doesn't work 
#autoreconf -I config -i -v

