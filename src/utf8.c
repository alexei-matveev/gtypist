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

#include "config.h"
#include "utf8.h"

#ifdef HAVE_PDCURSES
#include <curses.h>
#else
#include <ncursesw/ncurses.h>
#endif

#include <stdlib.h>
#include "gettext.h"
#define _(String) gettext (String)

void wideaddch(wchar_t c)
{
  cchar_t c2;
  wchar_t wc[2];
  int result;

  wc[0] = c;
  wc[1] = L'\0';

  result = setcchar(&c2, wc, 0, 0, NULL);
  if (result != OK)
    {
      fatal_error(_("error in setcchar()"), "?");      
    }
  add_wch(&c2);
}

void wideaddch_rev(wchar_t c)
{
  attron(A_REVERSE);
  wideaddch(c);
  attroff(A_REVERSE);
}

int mbslen(const char* str)
{
  return mbstowcs(NULL, str, 0);
}

wchar_t* widen(const char* text)
{
  int numChars = mbslen(text);
  wchar_t* wideText = malloc((numChars+1) * sizeof(wchar_t));
  int convresult = mbstowcs(wideText, text, numChars+1);
  if (convresult != numChars)
    fatal_error(_("couldn't convert UTF-8 to wide characters"), "?");
  return wideText;
}

/*
  Local Variables:
  tab-width: 8
  End:
*/
