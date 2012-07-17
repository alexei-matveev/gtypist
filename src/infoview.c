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

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#ifdef HAVE_PDCURSES
#include <curses.h>
#else
#include <ncursesw/ncurses.h>
#endif

#include "infoview.h"
#include "utf8.h"
#include "script.h"
#include "banner.h"
#include "gtypist.h"

#include "gettext.h"
#define _(String) gettext (String)

#define MIN( a, b ) ( ( a ) < ( b )? ( a ) : ( b ) )

static
void draw_frame(int x1, int y1, int x2, int y2)
{
    int x, y;

    attron (COLOR_PAIR (C_BANNER));

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

   attron (COLOR_PAIR (C_NORMAL));
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
    
    attron (COLOR_PAIR (C_BANNER));
    for (y = y1 + 1, scrollBarPosition = 1; y < y2; y++, scrollBarPosition++)
    {
        mvaddch(y, x2, 
                (scrollBarPosition >= scrollBarOffset &&
                 scrollBarPosition - scrollBarOffset <= scrollBarHeight)
                ? /*ACS_CKBOARD*/ ASCII_SPACE|A_REVERSE : ASCII_SPACE);
    }
    attron (COLOR_PAIR (C_NORMAL));
}

int do_beginner_infoview()
{
    const char* constMsg =
    /* TRANSLATORS: This message must not exceed 76 characters (UTF-8 code
       points, actually) wide or it will display incorrectly on an 80-character
       wide terminal. */
    _(  "Welcome to GNU Typist!\n"
        "\n"
        // 23456789012345678901234567890123456789012345678901234567890123456789012345
        "This message is meant to get you started using gtypist. Please read the\n"
        "manual (at http://www.gnu.org/software/gtypist/doc/, or type \"info gtypist\")\n"
        "for more in-depth information!\n"
        "(in particular, the manual describes how to resize GTypist's window).\n"
        "\n"
        "Use the arrow keys or PGUP/PGDN to scroll this window, SPACE/ENTER to start\n"
        "using gtypist, or ESCAPE to use gtypist and never show this dialog again.\n"
        "\n"
        "There are two types of typing exercises: \"drills\" and \"speed tests\". In a\n"
        "\"drill\", gtypist displays text in every other line on the screen and waits\n"
        "for the user to correctly type the exact same text in the intermediate\n"
        "lines. In a \"speed test\", gtypist displays text on the screen and waits\n"
        "for the user to correctly type the exact same text over the top.\n"
        "\n"
        "In both exercise types, you have to repeat the test if your error percentage\n"
        "is higher than a maximum error percentage. The default is 3%, but if this is\n"
        "too difficult you can change it by running gtypist with the \"-e <maxpct>\"\n"
        "command-line option.\n"
        "\n"
        "On most X11-based terminal emulators you can use Ctrl--/Ctrl-+ to resize the\n"
        "terminal window. On Windows, change the properties of the terminal window by\n"
        "clicking on the top left corner of the window and choosing 'Properties'.\n"
        "\n"
        "Several typing courses are available from the main menu. As well as the\n"
        "\"QWERTY\" courses, lessons are also available for other keyboard layouts,\n"
        "such as Dvorak and Colemak, and numeric keypads. There are also other\n"
        "lessons, besides the ones available on the main menu. To see a list of the\n"
        "other lessons, select \"More lessons...\" from the main menu. To use one\n"
        "of these other lessons, run gtypist with the name of the lesson file as a\n"
        "command-line argument. See the comments at the top of each .typ file for\n"
        "more information about the source and the author of the lessons. If you want\n"
        "to write your own lessons, look at the gtypist manual, it's really simple!\n"
        "\n"
        "If you have any questions about GNU Typist, please write to the gtypist\n"
        "mailing list at <bug-gtypist@gnu.org>."
    );

    char* msg; 
    char** msgLines;
    char* token;
    int numUsableLines, numMsgLines, i, j;
    int width, height, xOffset, yOffset;
    int firstLine, lastLine, msgLen;
    wchar_t ch;

    msg = strdup(constMsg);
	msgLen = strlen( msg );

    /* count the number of lines in msg */
    numMsgLines = 0;
    for (i = 0; i < msgLen; i++)
    {
        if (msg[i] == '\n')
        {
            numMsgLines++;
        }
    }
    if (i && msg[i - 1] != '\n')
    {
        numMsgLines++;
    }

    /* split into lines: use strsep instead of strtok because
       strtok cannot handle empty tokens */
    msgLines = malloc(numMsgLines * sizeof(char*));
	msgLines[0] = msg;
	for (i = j = 0; i < msgLen; i++)
	{
		if (msg[i] == '\n')
		{
			msg[i] = '\0';
			msgLines[++j] = &msg[ i + 1 ];
		}
	}

    width = 79 - 2; /* 2x window decoration */
    numUsableLines = LINES - 3; /* header, 2x window decoration */
    height = MIN(numUsableLines, numMsgLines + 2); /* frame top/bottom */
    xOffset = (COLS-width) / 2;
    yOffset = (LINES-height) / 2;

    /* variables for vertical scrolling */
    firstLine = 0;
    lastLine = (numMsgLines < height - 3) ? (numMsgLines - 1) : (height - 3);

    clear();
    banner("Beginner screen");
    draw_frame(xOffset, yOffset, xOffset + width, yOffset + height - 1);

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
        
        for (i = firstLine; i <= lastLine; i++)
        {
            move(yOffset + 1 + (i-firstLine), xOffset + 1);
            clrtoeol();
            mvwideaddstr(yOffset + 1 + (i-firstLine), xOffset + 1, msgLines[i]);
        }
        draw_scrollbar(xOffset + width, yOffset, yOffset + height - 1,
                       firstLine, lastLine, numMsgLines);
        mvwideaddstr(LINES - 1, 0,
                     _("Press SPACE or ENTER to start gtypist, or ESCAPE to disable this dialog"));
        get_widech(&ch);
    }

    /* free resources */
    free(msgLines);
    free(msg);

    return ch != 'q' && ch != 'Q';
}

