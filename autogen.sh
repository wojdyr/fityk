#!/bin/sh

# I'm using automake 1.9.6 and autoconf 2.61
set -x

autoreconf --install --verbose  \
&& ./configure "$@"

