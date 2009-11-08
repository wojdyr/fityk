#!/bin/sh

# I'm using automake 1.11, autoconf 2.64, libtool 2.2
set -x

autoreconf --install --verbose  \
&& ./configure "$@"

