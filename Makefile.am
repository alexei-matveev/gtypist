## Process this file with automake to produce Makefile.in

#
#	NOTE:  this file will probably be changed by gettextize, so don't
#	commit changed version into CVS.
#

AUTOMAKE_OPTIONS = 1.7.2 dist-bzip2

SUBDIRS= m4 po lessons tools doc intl

EXTRA_DIST = typefortune gtypist.1 typefortune.1 \
	configur.bat version.sh configur.bat.in autogen.sh \
	INSTALL.in TODO 

bin_PROGRAMS = gtypist
bin_SCRIPTS = typefortune
gtypist_SOURCES = gtypist.c cursmenu.c script.c error.c getopt.c getopt1.c \
		cursmenu.h  error.h  getopt.h  gettext.h  gtypist.h  script.h
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
