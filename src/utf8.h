/*
 * GNU Typist  - interactive typing tutor program for UNIX systems
 *
 * Copyright (C) 2003  GNU Typist Development Team <bug-gtypist@gnu.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef UTF8_H
#define UTF8_H

#include <ncursesw/ncurses.h>
#include <wchar.h>

// TODO: replace this with the proper include!
#define CCHARW_MAX	5
typedef struct
{
    attr_t	attr;
    wchar_t	chars[CCHARW_MAX];
#if 1
#undef NCURSES_EXT_COLORS
#define NCURSES_EXT_COLORS 20110404
    int		ext_color;	/* color pair, must be more than 16-bits */
#endif
}
cchar_t;

extern void wideaddch(wchar_t c);
extern void wideaddch_rev(wchar_t c);
extern int mbslen(char* str);

#endif /* !UTF8_H */
