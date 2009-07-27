#!/bin/sh

# I'm using automake 1.10, autoconf 2.63, libtool 2.2
set -x

autoreconf --install --verbose  \
&& ./configure "$@"

