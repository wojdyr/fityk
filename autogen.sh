#!/bin/sh
set -x
aclocal -I config
autoheader
automake --add-missing --copy
autoconf

