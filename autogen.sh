#!/bin/sh

# this is taken from pingus (http://pingus.seul.org)

rm -f config.cache
aclocal
autoheader
automake --add-missing
autoconf

# EOF #
