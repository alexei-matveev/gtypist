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

#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))


/* TODO: check terminal setup/reset */
char *do_menu (FILE *script, char *line)
{
  int num_items;
  char *data, *up, *title, **labels, **descriptions;
  int ch, i, j, k, idx;
  int cur_choice = 0, max_width, start_y, columns;
  int start_idx, end_idx; /* visible menu-items */
  int items_first_column, items_per_page, spacing;

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
  num_items = j;

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
  for (k = 0; k < num_items - 1; k++)
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
  if (up == NULL)
  {
    labels[k] = NULL;
    descriptions[k] = _("Exit"); 
  }
  else
  {
    labels[k] = up;
    descriptions[k] = _("Up");
  }
      
  /* get the longest description */
  max_width = 0;
  for (i = 0; i < num_items; i++)
    max_width = max (max_width, strlen (descriptions[i]));

  /* compute the number of columns */
  columns = 80 / (max_width + 1); /* maximum number of columns possible */
  while (columns > 1 && num_items / columns <= 3)
    /* it doesn't make sense to have i.e. 4 cols each having just one item! */
    columns--;

  /* how many item-rows are in the 1st column ? */
  items_first_column = num_items / columns;
  if (num_items % columns != 0)
    items_first_column++;

  /* compute start_y */
  if (items_first_column > LINES - 2)
    start_y = 1;
  else
    start_y = (LINES - items_first_column) / 2;

  /* compute spacing: space between columns and left-right end
     think about it: for columns=1: COLS = spacing + max_width + spacing
     => COLS = (columns+1) * spacing + columns * max_width <=> */
  spacing = (COLS - columns * max_width) / (columns + 1);

  /* compute items/page (for scrolling) */
  items_per_page = min (num_items, columns * 
			min (LINES - 2, items_first_column));

  /* the "viewport" (visible menu-items when scrolling)  */
  start_idx = 0;
  end_idx = items_per_page - 1;

  /* do clrscr only once */
  wclear (stdscr);
  wattron (stdscr, A_BOLD);
  mvwaddstr (stdscr, 0, 0, title);
  wattroff (stdscr, A_BOLD);
  do
    {
      /* (re)display the menu */
      for (i = 0; i < columns; i++)
	{
	  /* write 1 column */
	  for (j = 0; j < items_first_column &&
		 (idx = i * items_first_column + j + start_idx)
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
	  /* UP/DN */
	case KEY_UP:
	  cur_choice = max (0, cur_choice - 1);
	  if (cur_choice < start_idx) {
	    start_idx--; end_idx--;
	  }
	  break;
	case KEY_DOWN:
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

	case '\n':
	  break;

	default:
	  /*
	  waddstr(stdscr, "DEFAULT!!!");
	  wgetch (stdscr);*/
	}
      
    } while (ch != '\n');

  wattroff (stdscr, A_REVERSE);
  if (labels[cur_choice] != NULL)
  {
    seek_label (script, labels[cur_choice], line);
    get_script_line( script, line );
  }
  else
    do_exit (script);
  free (labels);
  free (descriptions);
  free (data);
 }
