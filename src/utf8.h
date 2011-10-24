/*
 * GNU Typist  - interactive typing tutor program for UNIX systems
 *
 * Copyright (C) 2011  GNU Typist Development Team <bug-gtypist@gnu.org>
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

#define _XOPEN_SOURCE_EXTENDED

#include <wchar.h>

extern wchar_t* widen(const char* text);
extern char* convertUTF8ToCurrentEncoding(const char* UTF8Input);
extern wchar_t* convertFromUTF8(const char* UTF8Text);
extern void mvwideaddstr(int y, int x, const char* UTF8Text);
extern void wideaddstr(const char* UTF8Text);
extern void wideaddstr_rev(const char* UTF8Text);
extern void wideaddch(wchar_t c);
extern void wideaddch_rev(wchar_t c);
extern int utf8len(const char* UTF8Text);

#endif /* !UTF8_H */
