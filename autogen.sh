#!/bin/sh

# I'm using automake 1.9.6, autoconf 2.61, libtool 1.5
set -x

autoreconf --install --verbose  \
&& ./configure "$@"

