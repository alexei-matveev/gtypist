#!/bin/sh
# Run this program to build the configuration
# files for GNU Typist:
# [*/]Makefile.in, aclocal.m4, configure,
# config.h.in, config.sub, config.guess

# WARNING: You need the following tool
# versions to run this script:
# autoconf-2.50, automake-1.4, m4-1.4, gettext 0.10.39

# Build documentation files

makeinfo doc/gtypist.texinfo -o doc/gtypist.info

# Build configuration files

rm -f config.cache
aclocal
autoheader
automake --add-missing
autoconf

