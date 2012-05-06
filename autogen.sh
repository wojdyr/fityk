#!/bin/sh

set -x

# choose one:
(cd doc && make)                  # build docs
#mkdir -p doc/html/placeholder    # do not build docs

# download and unpack cmpfit-1.2.tar.gz
# from http://www.physics.wisc.edu/~craigm/idl/cmpfit.html
curl -O http://www.physics.wisc.edu/~craigm/idl/down/cmpfit-1.2.tar.gz
tar xzf cmpfit-1.2.tar.gz
mkdir -p fityk/cmpfit/
cp -af cmpfit-1.2/mpfit.* fityk/cmpfit/

autoreconf --install --verbose  \
&& ./configure "$@"

