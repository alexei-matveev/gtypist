# Process this file with autoconf to produce a configure script.

AC_INIT([GNU Typist], [esyscmd(./version.sh)], [bug-gtypist@gnu.org], [gtypist])
AM_INIT_AUTOMAKE([1.8.2 gnu dist-bzip2])
AC_CONFIG_SRCDIR([src/gtypist.c])
AM_CONFIG_HEADER([config.h])

# Checks for programs

AC_PROG_AWK
AC_PROG_CC
AC_ISC_POSIX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_YACC

# Set of available languages.

ALL_LINGUAS="cs de es eu fi fr nl ru zh_CN zh_TW"

# Checks for libraries.

AC_CHECK_LIB(curses, wbkgdset,
	       [AC_DEFINE(HAVE_LIBCURSES, 1, [Is curses available])
	         HAVE_LIBCURSES=yes])

# Check for ncurses if curses is not found:
# i.e. on SuSE there are no ncurses* -> curses* links

AC_CHECK_LIB(ncurses, wbkgdset,
	[AC_DEFINE(HAVE_LIBNCURSES, 1, [Is curses available])
	         HAVE_LIBNCURSES=yes]
		 )

# Checks for header files.

AC_HEADER_STDC
AC_CHECK_HEADERS([unistd.h alloca.h argz.h errno.h fcntl.h langinfo.h libintl.h limits.h locale.h malloc.h stddef.h stdlib.h string.h strings.h sys/param.h unistd.h])

AC_CHECK_HEADER(curses.h, HAVE_CURSES_H=1)
AC_CHECK_HEADER(ncurses/curses.h, HAVE_NCURSES_H=1)

# We prefer ncurses to curses if both are found.
if test -n "$HAVE_NCURSES_H" -a -n "$HAVE_LIBNCURSES";  then
   LIBS="-lncurses $LIBS"
elif test -n "$HAVE_CURSES_H" -a -n "$HAVE_LIBCURSES"; then
   LIBS="-lcurses $LIBS"
else
   echo -e "Error:  both library and header files for either curses or ncurses"\
   	"library\nis required to build this package.  See INSTALL file for"\
	"further information."
   exit 1;
fi

# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.

AC_PROG_GCC_TRADITIONAL
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRTOD
AC_CHECK_FUNCS([__argz_count __argz_next __argz_stringify dcgettext getcwd getwd mempcpy memset munmap nl_langinfo setlocale stpcpy strcasecmp strchr strcspn strdup strstr strtoul])

# Internationalization macros

AM_GNU_GETTEXT_VERSION(0.12.1)
AM_GNU_GETTEXT

# Manual page generation

GEN_MAN="help2man ./gtypist -o gtypist.1"
AC_SUBST(GEN_MAN)

# This is necessary for tools/gtypist-mode.el

AM_PATH_LISPDIR

# Generate files

AC_CONFIG_FILES([lessons/Makefile tools/Makefile doc/Makefile src/Makefile
		Makefile intl/Makefile po/Makefile.in m4/Makefile])
AC_OUTPUT
