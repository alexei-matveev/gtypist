@echo Configuring gtypist 2.7beta1
@echo Using DJGPP and PDCURSES
@echo No Native Language Support
@echo Creating config.h
@echo /* config.h */ > config.h
@echo #undef ENABLE_NLS >>config.h
@echo #define PACKAGE "gtypist" >>config.h
@echo #define VERSION "2.7beta1" >>config.h
@echo Creating Makefile
@echo # Makefile > Makefile
@echo gtypist.exe: Makefile gtypist.c >> Makefile
@echo 	gcc -I. -static -O -DNCURSES -o gtypist gtypist.c getopt.c getopt1.c -lpdcurses -lpanel >> Makefile
