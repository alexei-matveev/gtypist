#include "config.h"
#include "error.h"
#include "script.h"
#ifdef HAVE_LIBCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif
#include "gettext.h"

char *argv0 = NULL;

/*
  handle fatal errors (pretty much any error) by dropping out
  of curses etc, and printing a complaint
  message that is already translated to the appropriate language 
*/
void 
fatal_error( char *message, char *line ) {

  /* stop curses */
  /* if ( cl_colour && has_colors() ) */
  wbkgdset( stdscr, 0 );
  clear(); refresh(); endwin();
  
  /* print out the error message and stop */
  fprintf( stderr, "%s: %s %d: %s", argv0, _("line"), global_line_counter,
	   message );
  if ( line != NULL )
    fprintf( stderr, ":\n%s\n", line );
  else
    fprintf( stderr, "\n" );
  exit( 1 );
}
