## Process this file with automake to produce Makefile.in

info_TEXINFOS=gtypist.texi gtypist-cs.texi
gtypist_TEXINFOS=gpl.texi fdl.texi

EXTRA_DIST=gtypist.html gtypist.cs.html

html_docs = gtypist.html gtypist.cs.html

.texi.html:
	$(MAKEINFO) --html --no-split -o gtypist.cs.html gtypist-cs.texi
	$(MAKEINFO) --html --no-split -o gtypist.html gtypist.texi

all: info html

.PHONY: html
html: version.texi version2.texi $(html_docs)

