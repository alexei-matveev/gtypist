/*
 * GNU Typist  - interactive typing tutor program for UNIX systems
 * 
 * Copyright (C) 2003  GNU Typist Development Team <gtypist-bug@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "cursmenu.h"
#include "script.h"
#ifdef HAVE_LIBCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif
#include "gettext.h"
#include "error.h"
#include "gtypist.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))

// This history list is needed in order to get the entire menu navigation
// working
typedef struct _MenuNode
{
   char *label;
   struct _MenuNode *next;
}
MenuNode;

// They are kept non-NULL as soon as the main menu is visited.
static MenuNode *start_node = NULL, *last_node = NULL;

static MenuNode *node_new ()
{
   MenuNode *mn = (MenuNode *) calloc (1, sizeof (MenuNode));
   if (!mn)
   {
      perror ("calloc");
      fatal_error ("internal error: malloc", NULL);
   }

   return mn;
}

static void node_delete (MenuNode *mn)
{
   if (mn -> label)
      free (mn -> label);

   free (mn);
}

// NOTE:  it is very ineffecient approach in general, but it is quite
// appropriate for this quick and dirty hack.
static void append_menu_history (const char *label)
{
   if (!label)
      label = "";

   // First check if we've already been here
   if (start_node)
   {
      MenuNode *mn = start_node;
      do
      {
	 if (!strcmp (label, (mn -> label)))
	 {
	    if (mn == last_node)
	       return;

	    // Go to the (same) old place
	    last_node = mn;
	    (last_node -> next) = NULL;

	    mn = (mn -> next);
	    while (mn)
	    {
	       MenuNode *t = (mn -> next);
	       node_delete (mn);
	       mn = t;
	    }

	    return;
	 }
	 
	 mn = (mn -> next);
      }
      while (mn);
   }
   
   // Ok, append it to the history
   if (!last_node)
      start_node = last_node = node_new ();
   else
   {
      (last_node -> next) = node_new ();
      last_node = (last_node -> next);
   }
 
   (last_node -> label) = strdup (label);
   if (!(last_node -> label))
   {
      perror ("strdup");
      fatal_error ("internal error: strdup", NULL);
   }
}

// Set the position of the script to the preceeding to the last_label,
// remove the last position of the history.
static void prepare_to_go_back (FILE *script)
{
   MenuNode *mn = start_node;

   if (!start_node)
      do_exit (script);	// No way back

   if (!(start_node -> next))
      do_exit (script);	// No way back too

   // Get the previous node
   while ((mn -> next) != last_node)
      mn = (mn -> next);

   if (!*(start_node -> next -> label))
      do_exit (script);

   seek_label (script, (mn -> label), (last_node -> label));
   node_delete (last_node);
   (mn -> next) = NULL;
   last_node = mn;
}

/* TODO: check terminal setup/reset */
char *do_menu (FILE *script, char *line)
{
  int num_items;
  char *data, *up, *title, **labels, **descriptions;
  int ch, i, j, k, idx;
  int cur_choice = 0, max_width, start_y, columns;
  int start_idx, end_idx; /* visible menu-items */
  int items_first_column, items_per_page, real_items_per_column, spacing;

  const int MENU_HEIGHT_MAX = LINES - 6;

  append_menu_history (__last_label);
  
  // Bind our former F12 key to the current menu
  bind_F12 (__last_label);

  data = buffer_command (script, line);
  /* e.g.:
   data=' "A course for the beginners"
   LESSON_1 "Lesson 1: home row keys"
   LESSON_2 "Lesson 2: some more keys"
   ' */

  /* data has a trailing '\n' => num_items = num_newlines - 1 
     (plus one item for UP or EXIT) */
  i = 0; j = 0;
  while (data[i] != '\0')
  {
    if (data[i++] == '\n')
      j++;
  }
//  num_items = j;
  num_items = j - 1;

  i = 0;
  /* get UP-label if present */
  up = NULL; /* up=NULL means top-level menu (exit-option in menu) */
  while (isspace(data[i]))
    i++;
  if (strncmp (data + i, "up=", 3) == 0 ||
      strncmp (data + i, "UP=", 3) == 0)
  { /* I expect to see <up=LABEL> */
    i += 3; /* start of up-label */
    up = data + i;
    while (data[i] != ' ')
      i++;
    data[i] = 0;
    if (strcmp (up, "_exit") == 0 ||
	strcmp (up, "_EXIT") == 0)
      up = NULL;
  }

  /* get title */
  while (data[i] != '"') /* find opening " */
    i++;
  i++;
  title = data + i;
  /* find closing ": the title may contain ", so
     we have to find the _last_ " */
  while (data[i] != '\n')
    i++;
  while (data[i] != '"')
    i--;
  data[i] = 0; /* terminate title-string */

  /* get menu-items */
  labels = (char**)malloc (sizeof (char*) * num_items);
  descriptions = (char**)malloc (sizeof (char*) * num_items);
  /* iterate through [0;num_items - 2] (the last item is for up/exit) */
  for (k = 0; k < num_items/* - 1*/; k++)
  {
    while (data[i] != '\n')
      i++;
    /* skip '\n' and other whitespace */
    while (isspace (data[i]))
      i++;
    /* get label, which ends when the description (enclosed in
       quotes) starts */
    labels[k] = data + i;
    while (data[i] != '"')
      i++;
    j = i + 1; /* remember this position: start of description */
    i--;
    while (isspace (data[i]))
      i--;
    data[i + 1] = 0; /* terminate label-string */
    /* get description (enclosed in double quotes) */
    i = j;
    descriptions[k] = data + i;
    /* look for closing quote: the description may contain "
       so we have to find the _last_ " */
    while (data[i] != '\n')
      i++;
    while (data[i] != '"') 
      i--;
    data[i] = 0; /* terminate description */
  }
/*  if (up == NULL)
  {
    labels[k] = NULL;
    descriptions[k] = _("Exit"); 
  }
  else
  {
    labels[k] = up;
    descriptions[k] = _("Up");
  }*/
      
  /* get the longest description */
  max_width = 0;
  for (i = 0; i < num_items; i++)
    max_width = max (max_width, strlen (descriptions[i]));

  /* compute the number of columns */
  columns = COLS / (max_width + 2); /* maximum number of columns possible */
  while (columns > 1 && num_items / columns <= 3)
    /* it doesn't make sense to have i.e. 4 cols each having just one item! */
    columns--;

  /* how many item-rows are in the 1st column ? */
  items_first_column = num_items / columns;
  if (num_items % columns != 0)
    items_first_column++;

  /* compute start_y */
  if (items_first_column > MENU_HEIGHT_MAX)
    start_y = 4;
  else
    start_y = (LINES - items_first_column) / 2;

  /* compute spacing: space between columns and left-right end
     think about it: for columns=1: COLS = spacing + max_width + spacing
     => COLS = (columns+1) * spacing + columns * max_width <=> */
  spacing = (COLS - columns * max_width) / (columns + 1);

  /* compute items/page (for scrolling) */
  items_per_page = min (num_items, columns * 
			min (MENU_HEIGHT_MAX, items_first_column));

  /* find # of visible items in column */
  real_items_per_column = items_per_page / columns;
  if (items_per_page % columns != 0)
    real_items_per_column++;

  /* the "viewport" (visible menu-items when scrolling)  */
  start_idx = 0;
  end_idx = items_per_page - 1;

  /* do clrscr only once */
  // Preserve the top banner.
  move (1, 0);
  clrtobot ();

  // The menu title
  wattron (stdscr, A_BOLD);
  attron (COLOR_PAIR (C_MENU_TITLE));
  mvwaddstr (stdscr, 2, (80 - strlen (title)) / 2, title);
  attron (COLOR_PAIR (C_NORMAL));
  wattroff (stdscr, A_BOLD);

  // The prompt at the bottom of the screen
  mvwaddstr (stdscr, LINES - 1, 0,
		_(
"Use arrowed keys to move around, "
"SPACE or RETURN to select and ESCAPE to go back")
		);
  
  do
    {
      /* (re)display the menu */
      for (i = 0; i < columns; i++)
	{
	  /* write 1 column */
	  for (j = 0; j < real_items_per_column &&
		 (idx = i * real_items_per_column + j + start_idx)
		 <= end_idx; 
	       j++)
	    {
	      if (idx == cur_choice)
		wattrset (stdscr, A_REVERSE);
	      else
		wattroff (stdscr, A_REVERSE);
	      /* the formula for start_x:
		 i=0: 1*spacing + 0*max_width
		 i=1: 2*spacing + 1*max_width
		 i=2: 3*spacing + 2*max_width
		 i=3: 4*spacing + 3*max_width
		 => (i+1)*spacing + i*max_width */
	      mvwaddstr (stdscr,
			 start_y + j,
			 (i + 1) * spacing + i * max_width,
			 descriptions[idx]);
	      for (k = max_width - strlen (descriptions[idx]); k > 0; k--)
		waddch (stdscr, ' ');
	    }
	}

      ch = wgetch (stdscr);
      switch (ch)
	{
	case KEY_UP:
	case 'K':
	case 'k':
	  cur_choice = max (0, cur_choice - 1);
	  if (cur_choice < start_idx) {
	    start_idx--; end_idx--;
	  }
	  break;
	case KEY_DOWN:
	case 'J':
	case 'j':
	  cur_choice = min (cur_choice + 1, num_items - 1);
	  if (cur_choice > end_idx) {
	    start_idx++; end_idx++;
	  }
	  break;

	case KEY_PPAGE:
	  k = start_idx;
	  start_idx = max (0, start_idx - items_per_page);
	  end_idx += start_idx - k;
	  cur_choice += start_idx - k;
	  break;
	case KEY_NPAGE:
	  k = end_idx;
	  end_idx = min (end_idx + items_per_page, num_items - 1);
	  start_idx += end_idx - k;
	  cur_choice += end_idx - k;
	  break;

	case KEY_RIGHT:
	case 'l':
	case 'L':
	  if (cur_choice + real_items_per_column < num_items)
	     cur_choice += real_items_per_column;
	  break;

	case '\n':
#ifdef DJGPP
	case '\x0D':
#endif
	case ASCII_SPACE:
	  ch = KEY_ENTER;
	case KEY_ENTER:
	  break;

	case KEY_LEFT:
	case 'h':
	case 'H':
	  if (cur_choice - real_items_per_column >= 0)
	     cur_choice -= real_items_per_column;
	  break;
	  
	case KEY_CANCEL: // anyone knows where is this key on a PC keyboard?
	case ASCII_ESC:
	case 'q':
	case 'Q':
	  prepare_to_go_back (script);
	  goto cleanup;

	default:
	  // printf ("libncurses think that it's key \\%o\n", ch);
	  break;
	}
      
    } while (ch != KEY_ENTER);

  wattroff (stdscr, A_REVERSE);
  if (labels[cur_choice] != NULL)
  {
    seek_label (script, labels[cur_choice], line);
    get_script_line( script, line );
  }
  else
    do_exit (script);

cleanup:
  free (labels);
  free (descriptions);
  free (data);
  return NULL;
}
