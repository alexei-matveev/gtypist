## Process this file with automake to produce Makefile.in

#
#	NOTE:  this file will probably be changed by gettextize, so don't
#	commit changed version into CVS.
#

bin_PROGRAMS = gtypist
gtypist_SOURCES = gtypist.c cursmenu.c script.c error.c getopt.c getopt1.c \
		cursmenu.h error.h getopt.h gettext.h gtypist.h script.h
gtypist_LDADD = @LIBINTL@

AM_CPPFLAGS = -I../intl \
  -DDATADIR=\"$(pkgdatadir)\" \
  -DLOCALEDIR=\"$(datadir)/locale\"

# GEN_MAN is a shell command that generates the man page

man_MANS = gtypist.1

GEN_MAN=@GEN_MAN@

gtypist.1: gtypist.c gtypist
	if (test ! -e gtypist.1) then $(GEN_MAN); fi
