/*
 * GNU Typist  - interactive typing tutor program for UNIX systems
 *
 * Copyright (C) 2012  GNU Typist Development Team <bug-gtypist@gnu.org>
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
//	Header for some interface details.

#include <malloc.h>
#include "banner.h"
#include "utf8.h"
#include "config.h"
#include "gtypist.h"
#include "script.h"

#ifdef HAVE_PDCURSES
#include <curses.h>
#else
#include <ncursesw/ncurses.h>
#endif

#include "gettext.h"
#define _(String) gettext (String)

// Display the top banner with the given text
void banner (const char *text)
{
   int colnum, brand_length, brand_position, text_length, text_position;

   // Get rid of spaces at the edges of the text
   while (isspace (*text))
      text ++;
   text_length = strlen (text);
   if (text_length > 0)
   {
      while (isspace (text [text_length - 1]))
      {
         text_length --;
	 if (! text_length)
	    break;
      }
   }

   brand_length = utf8len (PACKAGE) + utf8len (VERSION) + 3,
   brand_position = COLS - brand_length,
   text_position = ((COLS - brand_length) > text_length) ?
		(COLS - brand_length - text_length) / 2 : 0;

// TODO:  much of redundant output here...

   move (B_TOP_LINE , 0);
   attron (COLOR_PAIR (C_BANNER));
   for (colnum = 0; colnum < COLS; colnum++)
      ADDCH_REV (ASCII_SPACE);

   move (B_TOP_LINE, text_position);
   {
     wchar_t* wideText = convertFromUTF8(text);
     int numChars = wcslen(wideText);

     int i;
     for (i = 0; i < numChars; i++)
       wideaddch_rev(wideText[i]);
     free(wideText);
   }

   move (B_TOP_LINE, brand_position);
   attron (COLOR_PAIR (C_PROG_NAME));
   ADDCH_REV (' ');
   ADDSTR_REV (PACKAGE);
   ADDCH_REV (' ');
   attron (COLOR_PAIR (C_PROG_VERSION));
   ADDSTR_REV (VERSION);
   refresh ();
   attron (COLOR_PAIR (C_NORMAL));
}
