#!/bin/sh
set -x
aclocal-1.7 -I config -I /usr/local/share/aclocal
autoheader
automake-1.7 --add-missing --copy
autoconf

