## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.7.2 dist-bzip2

SUBDIRS= m4  m4  po lessons tools doc intl

EXTRA_DIST= config.rpath mkinstalldirs  config.rpath mkinstalldirs  config.rpath  typefortune gtypist.1 typefortune.1 \
config.rpath configur.bat version.sh configur.bat.in autogen.sh \
INSTALL.in TODO 

bin_PROGRAMS = gtypist
bin_SCRIPTS = typefortune
gtypist_SOURCES = gtypist.c getopt.h getopt.c getopt1.c gettext.h \
	cursmenu.h cursmenu.c script.h script.c error.h error.c
gtypist_LDADD = @LIBINTL@
man_MANS = gtypist.1 typefortune.1

AM_CPPFLAGS = -I./intl \
  -DDATADIR=\"$(pkgdatadir)\" \
  -DLOCALEDIR=\"$(datadir)/locale\"

# GEN_MAN is a shell command that generates the man page
GEN_MAN=@GEN_MAN@

gtypist.1: gtypist.c gtypist
	if (test ! -e gtypist.1) then $(GEN_MAN); fi

ACLOCAL_AMFLAGS = -I m4
