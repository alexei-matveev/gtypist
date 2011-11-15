# Process this file with autoconf to produce a configure script.

AC_INIT([GNU Typist], [esyscmd(./version.sh)], [bug-gtypist@gnu.org], [gtypist])
AM_INIT_AUTOMAKE([1.8.2 gnu dist-xz])
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

ALL_LINGUAS="cs de es eu fi fr nl pl ru tr zh_CN zh_TW"

# Checks for libraries.
AC_CHECK_LIB(ncursesw, add_wch, HAVE_LIBNCURSESW=1)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([unistd.h alloca.h argz.h errno.h fcntl.h langinfo.h libintl.h limits.h locale.h malloc.h stddef.h stdio_ext.h stdlib.h string.h strings.h sys/param.h unistd.h])

AC_CHECK_HEADER(ncursesw/ncurses.h, HAVE_NCURSESW_H=1)

# sanity check for libncursesw: 
if test -n "$HAVE_NCURSESW_H" -a -n "$HAVE_LIBNCURSESW";  then
   LIBS="-lncursesw $LIBS"
else
   echo -e "Error:  both library and header files for the ncursesw library\n"\
   	"are required to build this package.  See INSTALL file for"\
	"further information. On Debian/Ubuntu you need to install libncursesw5-dev."
   exit 1;
fi

# Checks for typedefs, structures, and compiler characteristics.

AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_TYPE_SIZE_T

# Checks for library functions.

AC_PROG_GCC_TRADITIONAL
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRTOD
AC_CHECK_FUNCS([__argz_count __argz_next __argz_stringify dcgettext getcwd getwd mempcpy memset munmap nl_langinfo setlocale stpcpy strcasecmp strchr strcspn strdup strstr strtoul])

# Internationalization macros

AM_GNU_GETTEXT_VERSION(0.12.1)
AM_GNU_GETTEXT

# Manual page generation

GEN_MAN="help2man ./gtypist -o gtypist.1 -n 'a typing tutor with lessons for different keyboards and languages'"
AC_SUBST(GEN_MAN)

# This is necessary for tools/gtypist-mode.el

AM_PATH_LISPDIR

# Generate files

AC_CONFIG_FILES([lessons/Makefile tools/Makefile doc/Makefile src/Makefile
		Makefile intl/Makefile po/Makefile.in m4/Makefile])
AC_OUTPUT
