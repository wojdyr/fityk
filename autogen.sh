#!/bin/sh

# I'm using automake 1.9.6 and autoconf 2.60

set -x

## the old way was:
#aclocal -I config 
#autoheader
#automake --add-missing --copy
#autoconf


autoreconf -i -v  \
&& ./configure "$@"

