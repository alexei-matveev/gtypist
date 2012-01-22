/*
 * GNU Typist  - interactive typing tutor program for UNIX systems
 *
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003
 *               Simon Baldwin (simonb@sco.com)
 * Copyright (C) 2003, 2004, 2008, 2009, 2011, 2012
 *               GNU Typist Development Team <bug-gtypist@gnu.org>
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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>

#ifdef HAVE_PDCURSES
#include <curses.h>
#else
#include <ncursesw/ncurses.h>
#endif

#include "infoview.h"
#include "utf8.h"
#include "script.h"

#include "gettext.h"
#define _(String) gettext (String)

static
void draw_frame(int x1, int y1, int x2, int y2)
{
    int x, y;
    // ACS_HLINE, ACS_VLINE, ACS_(UL|UR|LL|LR)CORNER
    mvaddch(y1, x1, ACS_ULCORNER);
    y = y1;
    for (x = x1 + 1; x < x2; x++) /* top edge */
        mvaddch(y, x, ACS_HLINE);
    
    mvaddch(y1, x2, ACS_URCORNER);
    x = x2;
    for (y = y1 + 1; y < y2; y++) /* right edge */
        mvaddch(y, x, ACS_VLINE);
    
    mvaddch(y2, x1, ACS_LLCORNER);
    y = y2;
    for (x = x1 + 1; x < x2; x++) /* bottom edge */
        mvaddch(y, x, ACS_HLINE);
    
    mvaddch(y2, x2, ACS_LRCORNER);
    x = x1;
    for (y = y1 + 1; y < y2; y++) /* left edge */
        mvaddch(y, x, ACS_VLINE);
}

static
void draw_scrollbar(int x2, int y1, int y2,
                    int firstLine, int lastLine, int numMsgLines)
{
    int windowHeight = lastLine - firstLine + 1;
    int scrollBarHeight =
        (int)(windowHeight * windowHeight/(float)numMsgLines + 0.5);
    int maxFirstLine = numMsgLines - windowHeight;
    int maxOffset = windowHeight - scrollBarHeight;
    int scrollBarOffset =
        (int)(firstLine/(float)maxFirstLine * maxOffset + 0.5);
    int y, scrollBarPosition;
    
    for (y = y1 + 1, scrollBarPosition = 1; y < y2; y++, scrollBarPosition++)
    {
        mvaddch(y, x2, 
                (scrollBarPosition >= scrollBarOffset &&
                 scrollBarPosition - scrollBarOffset <= scrollBarHeight)
                ? /*ACS_CKBOARD*/ ASCII_SPACE|A_REVERSE : ASCII_SPACE);
    }
}

int do_beginner_infoview()
{
    const char* constMsg = 
        _("Welcome to gtypist 2.10!\n"
          "\n"
          "This text is meant to get you started using gtypist. Please read the manual\n"
          "(TODO:URL or \"info gtypist\") for more in-depth information!\n"
          "Use the arrow keys/PGUP/PGDN to scroll this window, SPACE/ENTER to start\n"
          "using GTypist, or ESCAPE use GTypist and not show this dialog again.\n"
          "\n"
          "There are two types of typing exercises: \"drills\" and \"speed tests\":\n"
          "In a \"drill\", gtypist displays text in every other line on the\n"
          "screen, and waits for the user to correctly type the exact same text in\n"
          "the intermediate lines.\n"
          "In a \"speed test\", gtypist displays text on the screen, and waits for\n"
          "the user to correctly over-type the exact same text.\n"
          "In both exercise types, you have to repeat the test if your error percentage\n"
          "is higher than the maximum error percentage.\n"
          "\n"
          "If the default max. error percentage is too difficult: you can change it\n"
          "by running gtypist with the \"-e <maxpct>\" command line option.\n"
          "\n"
          "On most (UNIX) operating systems, you can use Ctrl--/Ctrl-+ to\n"
          "resize the terminal window. On Windows, change the properties of the\n"
          "terminal window by clicking on the top left corner of the window and\n"
          "choosing 'Properties'\n"
          "\n"
          "More lessons:\n"
          "------------\n"
          "... there are more lessons which can be accessed by starting\n"
          "gtypist with an argument, the name of the file:\n"
          "\n"
          " cs.typ            -  Czech lessons\n"
          " esp.typ           -  Spanish lessons\n"
          " ru.typ            -  Russian lessons\n"
          " ttde.typ          -  German lessons from tipptrainer 0.6.0\n"
          " ktbg.typ          -  Bulgarian lessons from KTouch 1.6\n"
          " ktbg_long.typ     -  Bulgarian lessons (longer) from KTouch 1.6\n"
          " ktde.typ          -  German lessons I from KTouch 1.6\n"
          " ktde2.typ         -  German lessons II from KTouch 1.6\n"
          " ktde_neo.typ      -  German lessons with NEO layout from KTouch 1.6\n"
          "  (see http://pebbles.schattenlauf.de/layout.php?language=de\n"
          " ktde_number.typ   -  German number keypad lessons from KTouch 1.6\n"
          " ktdk.typ          -  Danish lessons I from KTouch 1.6\n"
          " ktdk2.typ         -  Danish lessons II from KTouch 1.6\n"
          " ktdvorak.typ      -  Dvorak lessons from KTouch 1.6\n"
          "  (see http://en.wikipedia.org/wiki/Dvorak_Simplified_Keyboard)\n"
          " ktdvorak_es.typ   -  Spanish Dvorak lessons from KTouch 1.6\n"
          " ktdvorak_abcd.typ -  Dvorak lessons (basic) from KTouch 1.6\n"
          " kten.typ          -  English lessons from KTouch 1.6\n"
          " ktes.typ          -  Spanish lessons from KTouch 1.6\n"
          " ktes_cat.typ      -  Catalan lessons from KTouch 1.6\n"
          " ktfi.typ          -  Finnish lessons from KTouch 1.6\n"
          " ktfi_kids.typ     -  Finnish lessons for kids from KTouch 1.6\n"
          " ktfr.typ          -  French lessons I from KTouch 1.6\n"
          " ktfr2.typ         -  French lessons II from KTouch 1.6\n"
          " kthu.typ          -  Hungarian lessons I from KTouch 1.6\n"
          " kthu_expert.typ   -  Hungarian lessons II from KTouch 1.6\n"
          " ktit.typ          -  Italian lessons from KTouch 1.6\n"
          " ktnl.typ          -  Dutch lessons from KTouch 1.6\n"
          " ktnl_junior.typ   -  Dutch lessons for kids from KTouch 1.6\n"
          " ktno.typ          -  Norwegian lessons from KTouch 1.6\n"
          " ktpl.typ          -  Polish lessons from KTouch 1.6\n"
          " ktru.typ          -  Russian lessons from KTouch 1.6\n"
          " ktru_long.typ     -  Russian lessons (longer) from KTouch 1.6\n"
          " ktru_slava.typ    -  Russian lessons (hand-made) from KTouch 1.6\n"
          " ktsi.typ          -  Slovenian lessons from KTouch 1.6\n"
          " kttr.typ          -  Turkish lessons from KTouch 1.6\n"
          "\n"
          "See the comments at the top of each .typ file for more information\n"
          "about the source and the author of the lessons.\n"
          "\n"
          "If you want to write your own lessons, look at the gtypist manual,\n"
          "it's really simple! If you have any question, write to the\n"
          "gtypist mailing list: bug-gtypist@gnu.org\n"
          "\n"
            );
    
    char* msg;
    char** msgLines;
    char* token;
    int numUsableLines, numMsgLines, i, j;
    int width, height, xOffset, yOffset;
    int firstLine, lastLine;
    wchar_t ch;

    msg = strdup(constMsg);

    /* count the number of lines in msg */
    numMsgLines = 1;
    for (i = 0; i < strlen(msg); i++)
    {
        if (msg[i] == '\n')
        {
            numMsgLines++;
        }
    }

    msgLines = malloc(numMsgLines * sizeof(char*));
    /* split into lines: use strsep instead of strtok because
       strtok cannot handle empty tokens */
    i = 0;
    token = strsep(&msg, "\n");
    do
    {
        msgLines[i++] = strdup(token);
        token = strsep(&msg, "\n");
    } while (token != NULL);

    width = 79 - 2; /* 2x window decoration */
    numUsableLines = LINES - 3; /* header, 2x window decoration */
    height = MIN(numUsableLines, MAX(numMsgLines, 20));
    xOffset = (COLS-width) / 2;
    yOffset = 1; /* header above! */

    /* variables for vertical scrolling */
    firstLine = 0;
    lastLine = (numMsgLines < height - 3) ? (numMsgLines - 1) : (height - 3);

    ch = 0x00;
    while (ch != ASCII_SPACE && ch != ASCII_ESC && ch != ASCII_NL
           && ch != 'q' && ch != 'Q')
    {
        switch (ch)
        {
        case KEY_UP:
            if (firstLine > 0)
            {
                firstLine--;
                lastLine--;
            }
            break;
        case KEY_DOWN:
            if (lastLine + 1 < numMsgLines)
            {
                firstLine++;
                lastLine++;
            }
            break;
        case KEY_PPAGE: {
            int old_firstLine = firstLine;
            firstLine -= 10;
            if (firstLine < 0)
            {
                firstLine = 0;
            }
            lastLine += firstLine - old_firstLine;
            break;
        }
        case KEY_NPAGE:  {
            int old_lastLine = lastLine;
            lastLine += 10;
            if (lastLine + 1 >= numMsgLines)
            {
                lastLine = numMsgLines - 1;
            }
            firstLine += lastLine - old_lastLine;
            break;
        }
        }

        clear();
        draw_frame(xOffset, yOffset, xOffset + width, yOffset + height - 1);
        draw_scrollbar(xOffset + width, yOffset, yOffset + height - 1,
                       firstLine, lastLine, numMsgLines);
        for (i = firstLine; i <= lastLine; i++)
        {
            mvwideaddstr(yOffset + 1 + (i-firstLine), xOffset + 1, msgLines[i]);
        }
        mvwideaddstr(yOffset + height, 0,
                     _("Press SPACE or ENTER to start gtypist, or ESCAPE to disable this dialog"));
        get_widech(&ch);
    }

    /* free resources */
    for (i = 0; i < numMsgLines; i++)
    {
        free(msgLines[i]);
    }
    free(msgLines);
    free(msg);

    return ch != 'q' && ch != 'Q';
}

