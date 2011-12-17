#!/bin/sh

set -x
(cd doc && make)
autoreconf --install --verbose  \
&& ./configure "$@"

