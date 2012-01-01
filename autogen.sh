#!/bin/sh

set -x
(cd doc && make)
# download and unpack cmpfit-1.2.tar.gz
# from http://www.physics.wisc.edu/~craigm/idl/cmpfit.html
echo "copy content of cmpfit-1.2 (or later) to src/cmpfit/"
#rm -rf src/cmpfit/
#cp -r cmpfit-1.2/ src/cmpfit/
autoreconf --install --verbose  \
&& ./configure "$@"

