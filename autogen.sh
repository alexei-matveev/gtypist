#!/bin/sh
# Run this program to build the configuration
# files for GNU Typist:
# [*/]Makefile.in, aclocal.m4, configure,
# config.h.in, config.sub, config.guess
# INSTALL, configur.bat, doc/gtypist.info, lessons/gtypist.typ (maybe more)

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`autoconf' installed"
    echo "Get ftp://ftp.gnu.org/gnu/autoconf/autoconf-2.57.tar.bz2"
    echo "(or a newer version if it is available)"
    exit 1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`automake' installed."
    echo "Get ftp://ftp.gnu.org/gnu/automake/automake-1.7.6.tar.bz2"
    echo "(or a newer version if it is available)"
    exit 1
}

(gettext --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`gettext' installed."
    echo "Get ftp://ftp.gnu.org/gnu/gettext/gettext-0.11.5.tar.gz"
    echo "(or a newer version if it is available)"
    exit 1
}

(help2man --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`help2man' installed."
    echo "Get ftp://ftp.gnu.org/gnu/help2man/help2man-1.27.tar.gz"
    echo "(or a newer version if it is available)"
    exit 1
}


(makeinfo --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`texinfo' installed."
    echo "Get ftp://ftp.gnu.org/gnu/texinfo/texinfo-4.6.tar.bz2"
    echo "(or a newer version if it is available)"
    exit 1
}

# Generate lesson menus
(gawk --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: The cvs version requires \`gawk' (awk won't do)."
    echo "This is needed to build lessons/gtypist.typ."
    echo "Write to bug-gtypist@gnu.org if you have problems with this."
    exit 1
}
echo "creating lessons/gtypist.typ..."
(cd lessons && gawk -f ../tools/typcombine q.typ r.typ t.typ v.typ u.typ d.typ m.typ s.typ n.typ > gtypist.typ)


# Get version and write it in files
. ./version.sh

# make sure that there's no whitespace after the version number
# (in that case the awk command at the top of configure.in won't work)
VERSION_FROM_AWK="`cat version.sh | grep '^VERSION' | awk -F= '{print $2}'`"
if test "$VERSION_FROM_AWK" != "$VERSION"; then
    echo "There is whitespace around the version-number in version.sh. Please fix it."
    exit -1
fi

for file in configur.bat INSTALL
  do
  echo "creating $file..."
  if test $file -nt ${file}.in; then
      echo "*** \"$file\" has been modified,"
      echo "but it is generated from ${file}.in."
      echo "Please apply the changes to ${file}.in instead."
      exit -1;
  fi
  sed "s/@VERSION/$VERSION/g" ${file}.in > $file
  # TODO: this causes cvs to think that ${file}.in was modified...
  # solution: use a "stamp" file for each $file ?
  touch ${file}.in
done

# gettextize
echo "running gettextize...  Ignore non-fatal messages."
gettextize --force --copy --intl --no-changelog

# Build configuration files

echo "creating build configuration files..."
aclocal -I m4
autoheader
automake --add-missing

autoconf
if [ $? != 0 ]; then
    echo -e \
    	"--------------------------------------------------------------------" \
    	"\nSomething was wrong, autoconf failed." \
	"\nConsult notes on how to build if from CVS in the README.CVS file," \
	"\nif you don't know how to resolve this."
    exit 1
fi
    

if test -z "$*"; then
    echo
    echo "**Warning**: I am going to run \`configure' with no arguments."
    echo "If you wish to pass any to it, please specify them on the"
    echo \`$0\'" command line."
fi

echo
echo running ./configure "$@"...
echo
./configure "$@"

# Run make
# Needed to generate the version.texi file

make

# Generate documentation now that version.texi exists

cd doc
echo "creating doc/gtypist.info..."
makeinfo -Idoc gtypist.texi -o gtypist.info
echo "creating doc/gtypist.cs.info..."
makeinfo -Idoc gtypist.cs.texi -o gtypist.cs.info
# Bug: '--html --no-header' doesn't work without '--no-split'
# Reported to texinfo developers.
echo "creating doc/gtypist.html..."
makeinfo --html --no-header --no-split gtypist.texi -o gtypist.html 
echo "creating doc/gtypist.cs.html..."
makeinfo --html --no-header --no-split gtypist.cs.texi -o gtypist.cs.html 
cd ..

# Create the source packages

make dist

