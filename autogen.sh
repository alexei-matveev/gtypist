#!/bin/sh
# Run this program to build the configuration
# files for GNU Typist:
# [*/]Makefile.in, aclocal.m4, configure

# Build documentation files

makeinfo doc/gtypist.texinfo -o doc/gtypist.info

# Build configuration files

rm -f config.cache
aclocal
autoheader
automake --add-missing
autoconf

