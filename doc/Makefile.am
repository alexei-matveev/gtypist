## Process this file with automake to produce Makefile.in

info_TEXINFOS=gtypist.texi gtypist.cs.texi gtypist.es.texi
gtypist_TEXINFOS=gpl.texi fdl.texi

EXTRA_DIST=gtypist.html gtypist.cs.html gtypist.es.html

html_docs=gtypist.html gtypist.cs.html gtypist.es.html

.texi.html:
	$(MAKEINFO) --html --no-split --no-headers -o gtypist.html gtypist.texi
	$(MAKEINFO) --html --no-split --no-headers -o gtypist.cs.html gtypist.cs.texi
	$(MAKEINFO) --html --no-split --no-headers -o gtypist.es.html gtypist.es.texi

all: info html

.PHONY: html
html: version.texi version2.texi version3.texi $(html_docs)

