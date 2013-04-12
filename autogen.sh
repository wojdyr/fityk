#!/bin/sh

set -x

# choose one:
(cd doc && make)                  # build docs
#mkdir -p doc/html/placeholder    # do not build docs

autoreconf --install --verbose  \
&& ./configure "$@"

