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

#include <time.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <locale.h>
#include <wctype.h>
#ifndef MINGW
#include <langinfo.h>
#endif

#include "cursmenu.h"
#include "script.h"
#include "error.h"
#include "gtypist.h"
#include "utf8.h"
#include "infoview.h"
#include "speedbox.h"
#include "cmdline.h"

#include "gettext.h"
#define _(String) gettext (String)

/* VERSION and PACKAGE defined in config.h */

char *COPYRIGHT;

char* locale_encoding; /* current locale's encoding */
int isUTF8Locale; /* does the current locale have a UTF-8 encoding? */

/* character to be display to represent "enter key" */
/* TODO: this requires beginner mode!
#define RETURN_CHARACTER 0x000023CE */
#define RETURN_CHARACTER 0x00000020 

/* a definition of a boolean type */
#ifndef bool
#define bool			int
#endif

/* mode indicator strings */
char *MODE_TUTORIAL;
char *MODE_QUERY;
char *MODE_DRILL;
char *MODE_SPEEDTEST;

/* yes/no responses and miscellanea */
#define	QUERY_Y			'Y'
#define	QUERY_N			'N'
#define	DRILL_CH_ERR		'^'
#define	DRILL_NL_ERR		'^'
char *WAIT_MESSAGE;
char *ERROR_TOO_HIGH_MSG;
char *SKIPBACK_VIA_F_MSG;
char *REPEAT_NEXT_EXIT_MSG;
char *REPEAT_EXIT_MSG;
char *CONFIRM_EXIT_LESSON_MSG;
char *NO_SKIP_MSG;
wchar_t *YN;
wchar_t *RNE;

#ifdef MINGW
#define DATADIR "lessons"
#else
#ifndef DATADIR
#define DATADIR "."
#endif
#endif

#define	DEFAULT_SCRIPT      "gtypist.typ"

#ifdef MINGW
#define BESTLOG_FILENAME    "gtypist-bestlog"
#else
#define BESTLOG_FILENAME    ".gtypist-bestlog"
#endif


/* some colour definitions */
static short	colour_array[] = {
  COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
  COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE };
#define	NUM_COLOURS		(sizeof( colour_array ) / sizeof( short ))

#ifdef MINGW
#define MIN( a, b ) ( ( a ) < ( b )? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) > ( b )? ( a ) : ( b ) )
#endif

/* command line and config file options */
static struct gengetopt_args_info cl_args;      /* program options */
static int	cl_fgcolour = 7;		/* fg colour */
static int	cl_bgcolour = 0;		/* bg colour */
static int	cl_banner_bg_colour = 0;	/* banner bg colorr */
static int	cl_banner_fg_colour = 6;	/* banner fg colour */
static int	cl_prog_name_colour = 5;	/* program name colour */
static int 	cl_prog_version_colour = 1;     /* program version colour */

/* a few global variables */
static bool	global_resp_flag = TRUE;
static char	global_prior_command = C_CONT;

static float	global_error_max = -1.0;
static bool	global_error_max_persistent = FALSE;

static struct	label_entry *global_on_failure_label = NULL;
static bool	global_on_failure_label_persistent = FALSE;

static char 	*global_script_filename = NULL;

static char	*global_home_env = NULL;

/* a global area for associating function keys with labels */
#define NFKEYS			12		/* num of function keys */
static char	*fkey_bindings[ NFKEYS ] =
  { NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL };
/* table of pseudo-function keys, to allow ^Q to double as Fkey1, etc */
#define	CTRL_OFFSET		0100		/* ctrl keys are 'X' - 0100 */
static	char	pfkeys[ NFKEYS ] =
  { 'Q'-CTRL_OFFSET, 'W'-CTRL_OFFSET, 'E'-CTRL_OFFSET, 'R'-CTRL_OFFSET,
    'T'-CTRL_OFFSET, 'Y'-CTRL_OFFSET, 'U'-CTRL_OFFSET, 'I'-CTRL_OFFSET,
    'O'-CTRL_OFFSET, 'P'-CTRL_OFFSET, 'A'-CTRL_OFFSET, 'S'-CTRL_OFFSET };

static bool user_is_always_sure = FALSE;

/* prototypes */

static int getch_fl( int cursor_char );
static bool wait_user (FILE *script, char *message, char *mode );
static void display_speed( int total_chars, long elapsed_time, int errcount );
static void do_keybind( FILE *script, char *line );
static void do_tutorial( FILE *script, char *line );
static void do_instruction( FILE *script, char *line );
static int is_error_too_high( int chars_typed, int errors );
static void do_drill( FILE *script, char *line );
static void do_speedtest( FILE *script, char *line );
static void do_clear( FILE *script, char *line );
static void do_goto( FILE *script, char *line, bool flag );
static char do_query_repeat( FILE *script, bool allow_next );
static bool do_query_simple( char *text );
static bool do_query( FILE *script, char *line );
static void do_error_max_set( FILE *script, char *line );
static void do_on_failure_label_set( FILE *script, char *line );
static void parse_file( FILE *script, char *label );
static void catcher( int signal );
static FILE *open_script( const char *filename );
static void do_bell();
static bool get_best_speed( const char *script_filename,
			    const char *excersise_label, double *adjusted_cpm );
static void put_best_speed( const char *script_filename,
			    const char *excersise_label, double adjusted_cpm );
void get_bestlog_filename( char *filename );

void bind_F12 (const char *label)
{
  if (!label)
     return;

  if (fkey_bindings [11])
     free (fkey_bindings [11]);
  fkey_bindings [11] = strdup (label);
  if (! fkey_bindings [11])
  {
       perror ("strdup");
       fatal_error (_("internal error: strdup"), label);
  }
}

/*
  getch() that does a flashing cursor; some xterms seem to be
  unwilling to do A_BLINK, however, the program needs some
  way to separate out the inverse char cursor from the inverse
  char mistyping indications.  And to complicate things, some
  xterms seem not make the cursor invisible either.
*/
static int
getch_fl( int cursor_char )
{
  int	y, x;				/* saved cursor posn */
  int	return_char;			/* return value */
  bool	alternate = FALSE;		/* flashes control */

  /* save the cursor position - we're going to need it */
  getyx( stdscr, y, x );

  /* if no cursor then do our best not to show one */
  if ( cursor_char == ASCII_NULL )
    {
      /* degrade to cursor-less getch */
      curs_set( 0 ); refresh();
      move( LINES - 1, COLS - 1 );
      cbreak();
      get_widech(&return_char);
      move( y, x );
    }
  else
    {
      /* produce a flashing cursor, or not, as requested */
      if ( !cl_args.term_cursor_flag ) {
        /* go for the flashing block here */
        wideaddch_rev(cursor_char);
        curs_set( 0 ); refresh();
        move( LINES - 1, COLS - 1 );
        if ( ( cl_args.curs_flash_arg / 2 ) > 0 )
          {
            halfdelay( cl_args.curs_flash_arg / 2 );
            while ( get_widech(&return_char) == ERR )
              {
                move( y, x );
                if ( alternate )
                  wideaddch_rev(cursor_char);
                else
                  wideaddch(cursor_char);
                move( LINES - 1, COLS - 1 );
                alternate = !alternate;
              }
          }
        else
          {
            cbreak(); 
            get_widech(&return_char);
          }
        move( y, x );
        wideaddch(cursor_char);
        move( y, x );
      }
      else
        {
          /* just use the terminal's cursor - this is easy */
          curs_set( 1 ); refresh();
          cbreak(); //return_char = getch();
          get_widech(&return_char);
          curs_set( 0 ); refresh();
        }
    }

  /* return what key was pressed */
  return ( return_char );
}

/*
  wait for a nod from the user before continuing. In MODE_TUTORIAL only, TRUE is
  returned if the user pressed escape to indicate that seek_label was called
*/
static bool wait_user (FILE *script, char *message, char *mode)
{
  int	resp;			/* response character */
  bool	seek_done = FALSE;	/* was seek_label called? */

  /* move to the message line print a prompt */
  move( MESSAGE_LINE, 0 ); clrtoeol();
  move( MESSAGE_LINE, COLS - utf8len( mode ) - 2 );
  wideaddstr_rev(mode);
  move( MESSAGE_LINE, 0 );
  wideaddstr_rev(message);

  do {
    resp = getch_fl (ASCII_NULL);

    /* in tutorial mode only, escape has the special purpose that we exit to a
       menu (or quit if there is none) */
    if (resp == ASCII_ESC && mode == MODE_TUTORIAL)
    {
      // Return to the last F12-binded location
      if( fkey_bindings[ 11 ] && *( fkey_bindings[ 11 ] ) )
	{
          seek_label( script, fkey_bindings[ 11 ], NULL );
          seek_done = TRUE;
	}
      else
	do_exit( script );
      break;
    }
  } while (resp != ASCII_NL && resp != ASCII_SPACE && resp != ASCII_ESC);

  /* clear the message line */
  move( MESSAGE_LINE, 0 ); clrtoeol();

  return seek_done;
}


/*
  calculate and display speed and accuracy from a drill or speed test
*/
static void display_speed( int total_chars, long elapsed_time, int errcount ) {
  double	test_time;			/* time in minutes */
  double	cpm, adjusted_cpm;		/* speeds in CPM */
  bool		had_best_speed = FALSE;		/* already had a p.best? */
  bool		new_best_speed = FALSE;		/* have we beaten it? */
  double	best_cpm;			/* personal best speed in CPM */
  char		*raw_speed_str, *adj_speed_str, *best_speed_str;

  /* calculate the speeds */
  test_time = (double)elapsed_time / (double)60.0;
  if( elapsed_time > 0 )
    {
      /* calculate speed values */
      cpm = (double)total_chars / test_time;
      adjusted_cpm = (double)( total_chars - ( errcount * 5 ) ) / test_time;

      /* limit speed values */
      cpm = MIN( cpm, 9999.99 );
      adjusted_cpm = MAX( MIN( adjusted_cpm, 9999.99 ), 0 );

      /* remove errors in adjusted speed */
      if( adjusted_cpm < 0.01 )
	adjusted_cpm = 0;
    }
  else
    /* unmeasurable elapsed time - use big numbers */
    cpm = adjusted_cpm = (double)9999.99;

  /* obtain (and update?) a personal best speed */
  if( cl_args.personal_best_flag )
    {
      had_best_speed =
	get_best_speed( global_script_filename, __last_label, &best_cpm );
      new_best_speed = ( !had_best_speed || adjusted_cpm > best_cpm ) &&
	!is_error_too_high( total_chars, errcount );
      if( new_best_speed )
	put_best_speed( global_script_filename, __last_label, adjusted_cpm );
    }

  /* draw speed box */
  do_speed_box( total_chars, errcount, cpm, adjusted_cpm,
		cl_args.scoring_arg == scoring_arg_cpm? TRUE : FALSE,
		had_best_speed, new_best_speed, best_cpm );
}

/*
  bind a function key to a label
*/
static void
do_keybind( FILE *script, char *line ) {
  int	fkey;				/* function key number */
  char	*label;				/* associated label */

  /* extract the fkey number and label string, and check
     the syntax and correctness of the mappings */
  label = (char*)malloc( strlen(SCR_DATA( line )) + 1 );
  if ( sscanf( SCR_DATA( line ), "%d:%s", &fkey, label ) != 2 )
    fatal_error( _("invalid key binding"), line );
  if ( fkey < 1 || fkey > NFKEYS )
    fatal_error( _("invalid function key number"), line );

  /* free the previous binding malloced data */
  if ( fkey_bindings[ fkey - 1 ] != NULL )
    {
      free( fkey_bindings[ fkey - 1 ] );
      fkey_bindings[ fkey - 1 ] = NULL;
    }

  /* add the association to the array, or unbind the association
     if the target is the special label "NULL" (ugh - hacky) */
  if ( strcmp( label, "NULL" ) != 0 && strcmp( label, "null" ) != 0 )
    fkey_bindings[ fkey - 1 ] = label;
  else
    free( label );

  /* get the next command */
  get_script_line( script, line );
  global_prior_command = C_KEYBIND;
}


/*
  print the given text onto the screen
*/
static void
do_tutorial( FILE *script, char *line ) {
  int	linenum;		/* line counter */
  bool  seek_done = FALSE;      /* was there a seek_label before exit? */

  /* start at the top of the screen, and clear it */
  linenum = T_TOP_LINE;
  move( linenum, 0 ); clrtobot();

  /* output this line, and each continuation line read */
  do
    {
      if ( linenum >= LINES - 1 )
	fatal_error( _("data exceeds screen length"), line );
      move( linenum, 0 );
      /* ADDSTR( SCR_DATA( line )); */
      wideaddstr(SCR_DATA( line ));
      get_script_line( script, line );
      linenum++;
    }
  while ( SCR_COMMAND( line ) == C_CONT && ! feof( script ));

  /* wait for a return, unless the next command is a query,
     when we can skip it to save the user keystrokes */
  if ( SCR_COMMAND( line ) != C_QUERY )
    seek_done = wait_user (script, WAIT_MESSAGE, MODE_TUTORIAL);
  global_prior_command = C_TUTORIAL;

  /* if seek_label has been called (in wait_user) then we need to read in the
     next line of the script in to `line` */
  if (seek_done)
    get_script_line( script, line );
}


/*
  print up a line, at most two, usually followed by a drill or a speed test
*/
static void
do_instruction( FILE *script, char *line ) {

  /* move to the instruction line and output the first bit */
  move( I_TOP_LINE, 0 ); clrtobot();
  ADDSTR( SCR_DATA( line ));
  get_script_line( script, line );

  /* if there is a second line then output that too */
  if ( SCR_COMMAND( line ) == C_CONT && ! feof( script ))
    {
      move( I_TOP_LINE + 1, 0 );
      ADDSTR( SCR_DATA( line ) );
      get_script_line( script, line );
    }

  /* if there is a third line then complain */
  if ( SCR_COMMAND( line ) == C_CONT && ! feof( script ))
    fatal_error( _("instructions are limited to two lines"), line );
  global_prior_command = C_INSTRUCTION;
}

/*
  Calculate whether a drill's error rate is too high, keeping in mind
  rounding of output to a single decimal place.
*/
static int
is_error_too_high( int chars_typed, int errors ) {
  /* (double)100.0*(double)errcount / (double)total_chars ) */
  double err_max, err;
  char buf[BUFSIZ];

  /* Calculate error rates */
  err_max = (double)global_error_max;
  err = (double)100.0*(double)errors / (double)chars_typed;

  /* We need to use the same kind of rounding used by printf. */
  sprintf(buf, "%.1f", err_max);
  sscanf(buf, "%lf", &err_max);
  sprintf(buf, "%.1f", err);
  sscanf(buf, "%lf", &err);
  return err > err_max;
}

/*
  execute a typing drill
*/
static void
do_drill( FILE *script, char *line ) {

  int	errors = 0;		 /* error count */
  int	linenum;		 /* line counter */
  char	*data = NULL;		 /* data string */
  int	lines_count = 0;	 /* measures drill length */
  int	rc;			 /* curses char typed */
  wchar_t  *widep, *wideData;
  long	start_time=0, end_time;	 /* timing variables */
  char	message[MAX_WIN_LINE];	 /* message buffer */
  char	drill_type;		 /* note of the drill type */
  int	chars_typed;		 /* count of chars typed */
  int	chars_in_the_line_typed;
  bool  seek_done = FALSE;       /* was there a seek_label before exit? */
  int	error_sync;		 /* error resync state */

  /* note the drill type to see if we need to make the user repeat */
  drill_type = SCR_COMMAND( line );

  /* get the complete exercise into a single string */
  data = buffer_command( script, line );

  /* get the exercise as a wide string */
  wideData = convertFromUTF8(data);
  int numChars = wcslen(wideData);

  /* count the lines in this exercise, and check the result
     against the screen length */
  for ( widep = wideData, lines_count = 0; *widep != ASCII_NULL; widep++ )
    {
      if ( *widep == ASCII_NL)
        lines_count++;
    }
  if ( DP_TOP_LINE + lines_count * 2 > LINES )
    fatal_error( _("data exceeds screen length"), line );

  /* if the last command was a tutorial, ensure we have
     the complete screen */
  if ( global_prior_command == C_TUTORIAL )
    {
      move( T_TOP_LINE, 0 ); clrtobot();
    }

  while (1)
    {

      /* display drill pattern */
      linenum = DP_TOP_LINE;
      move( linenum, 0 ); clrtobot();
      for ( widep = wideData; *widep != ASCII_NULL; widep++ )
        {
          if ( *widep != ASCII_NL )
            wideaddch(*widep);
          else
          {
              /* emit return character */
              wideaddch(RETURN_CHARACTER);

              /* newline - move down the screen */
              linenum++; linenum++;	/* alternate lines */
              move( linenum, 0 );
          }
        }
      move( MESSAGE_LINE, COLS - utf8len( MODE_DRILL ) - 2 );
      ADDSTR_REV( MODE_DRILL );

      /* run the drill */
      linenum = DP_TOP_LINE + 1;
      move( linenum, 0 );
      for ( widep = wideData; *widep == ASCII_SPACE && *widep != ASCII_NULL; widep++ )
        wideaddch(*widep);

      for ( chars_typed = 0, errors = 0, error_sync = 0,
              chars_in_the_line_typed = 0;
            *widep != ASCII_NULL; widep++ )
        {
          do
            {
              rc = getch_fl (chars_in_the_line_typed >= COLS ? *(widep + 1) :
                             (*widep == ASCII_TAB ? ASCII_TAB : ASCII_SPACE));
            }
          while ( rc == KEY_BACKSPACE || rc == ASCII_BS || rc == ASCII_DEL );

          /* start timer on first char entered */
          if ( chars_typed == 0 )
            start_time = (long)time( NULL );
          chars_typed++;
          error_sync--;

          /* ESC is "give up"; ESC at beginning of exercise is "skip lesson"
             (this is handled outside the for loop) */
          if ( rc == ASCII_ESC )
            break;

          /* check that the character was correct */
          if ( rc == *widep || ( cl_args.word_processor_flag &&
				 rc == ASCII_SPACE && *widep == ASCII_NL ))
            {
              if (cl_args.word_processor_flag && rc == ASCII_SPACE &&
		  *widep == ASCII_NL)
                chars_in_the_line_typed = 0;
              else
                {
                  if (rc != ASCII_NL)
                    {
                      wideaddch(rc);
                      chars_in_the_line_typed ++;
                    }
                  else
                    {
                      wideaddch(RETURN_CHARACTER);
                      chars_in_the_line_typed = 0;
                    }
                }
            }
          else
            {
              /* try to sync with typist behind */
              if ( error_sync >= 0 && widep > wideData && rc == *(widep-1) )
                {
                  widep--;
                  continue;
                }

              if (chars_in_the_line_typed < COLS)
                {
                  wideaddch_rev( *widep == ASCII_NL ? DRILL_NL_ERR :
                                 (*widep == ASCII_TAB ?
                                  ASCII_TAB : (cl_args.show_errors_flag ?
                                               rc : DRILL_CH_ERR)));
                  chars_in_the_line_typed ++;
                }

              if (*widep == ASCII_NL)
                chars_in_the_line_typed = 0;

              if ( ! cl_args.silent_flag )
                {
                  do_bell();
                }
              errors++;
              error_sync = 1;

              /* try to sync with typist ahead */
              if ( rc == *(widep+1) )
                {
                  ungetch( rc );
                  error_sync++;
                }
            }

          /* move screen location if newline */
          if ( *widep == ASCII_NL )
            {
              linenum++; linenum++;
              move( linenum, 0 );
            }

          /* perform any other word processor like adjustments */
          if ( cl_args.word_processor_flag )
            {
              if ( rc == ASCII_SPACE )
                {
                  while ( *(widep+1) == ASCII_SPACE
                          && *(widep+1) != ASCII_NULL )
                    {
                      widep++;
                      wideaddch(*widep);
                      chars_in_the_line_typed ++;
                    }
                }
              else if ( rc == ASCII_NL )
                {
                  while ( ( *(widep+1) == ASCII_SPACE
                            || *(widep+1) == ASCII_NL )
                          && *(widep+1) != ASCII_NULL )
                    {
                      widep++;
                      wideaddch(*widep);
                      chars_in_the_line_typed ++;
                      if ( *widep == ASCII_NL ) {
                        linenum++; linenum++;
                        move( linenum, 0 );
                        chars_in_the_line_typed = 0;
                      }
                    }
                }
              else if ( isalpha(*widep) && *(widep+1) == ASCII_DASH
                        && *(widep+2) == ASCII_NL )
                {
                  widep++;
                  wideaddch(*widep);
                  widep++;
                  wideaddch(*widep);
                  linenum++; linenum++;
                  move( linenum, 0 );
                  chars_in_the_line_typed = 0;
                }
            }
        }

      /* ESC not at the beginning of the lesson: "give up" */
      if ( rc == ASCII_ESC && chars_typed != 1)
        continue; /* repeat */

      /* skip timings and don't check error-pct if exit was through ESC */
      if ( rc != ASCII_ESC )
        {
          /* display timings */
          end_time = (long)time( NULL );
          if ( ! cl_args.notimer_flag )
            {
              display_speed( chars_typed, end_time - start_time,
                             errors );
            }

          /* check whether the error-percentage is too high (unless in d:) */
          if (drill_type != C_DRILL_PRACTICE_ONLY &&
              is_error_too_high(chars_typed, errors))
            {
              sprintf( message, ERROR_TOO_HIGH_MSG, global_error_max );
              wait_user (script, message, MODE_DRILL);

              /* check for F-command */
              if (global_on_failure_label != NULL)
                {
                  /* move to the label position in the file */
                  if (fseek(script, global_on_failure_label->offset, SEEK_SET )
                      == -1)
                    fatal_error( _("internal error: fseek"), NULL );
                  global_line_counter = global_on_failure_label->line_count;
                  /* tell the user about the misery :) */
                  sprintf(message,SKIPBACK_VIA_F_MSG,
                          global_on_failure_label->label);
                  /* reset value unless persistent */
                  if (!global_on_failure_label_persistent)
                    global_on_failure_label = NULL;
                  wait_user (script, message, MODE_DRILL);
                  seek_done = TRUE;
                  break;
                }

              continue;
            }
        }

      /* ask the user whether he/she wants to repeat or exit */
      if ( rc == ASCII_ESC && cl_args.no_skip_flag ) /* honor --no-skip */
        rc = do_query_repeat (script, FALSE);
      else
        rc = do_query_repeat (script, TRUE);
      if (rc == 'E') {
        seek_done = TRUE;
        break;
      }
      if (rc == 'N')
        break;

    }

  /* free the malloced memory */
  free( data );
  free( wideData );

  /* reset global_error_max */
  if (!global_error_max_persistent)
    global_error_max = cl_args.max_error_arg;

  /* buffer_command takes care of advancing `script' (and setting
     `line'), so we only do if seek_label had been called (in
     do_query_repeat or due to a failure and an F: command) */
  if (seek_done)
    get_script_line( script, line );
  global_prior_command = drill_type;
}


/*
  execute a timed speed test
*/
static void
do_speedtest( FILE *script, char *line ) {
  int	errors = 0;		 /* error count */
  int	linenum;		 /* line counter */
  char	*data = NULL;		 /* data string */
  int	lines_count = 0;	 /* measures exercise length */
  int	rc;			 /* curses char typed */
  wchar_t  *widep, *wideData;
  long	start_time=0, end_time;	 /* timing variables */
  char	message[MAX_WIN_LINE];	 /* message buffer */
  char	drill_type;		 /* note of the drill type */
  int	chars_typed;		 /* count of chars typed */
  bool  seek_done = FALSE;       /* was there a seek_label before exit? */
  int	error_sync;		 /* error resync state */

  /* note the drill type to see if we need to make the user repeat */
  drill_type = SCR_COMMAND( line );

  /* get the complete exercise into a single string */
  data = buffer_command( script, line );

  wideData = convertFromUTF8(data);
  int numChars = wcslen(wideData);

  /*
    fprintf(F, "convresult=%d\n", convresult);
    fprintf(F, "wideData='%ls'\n", wideData);
    int i;
    for (i = 0; i <= numChars; i++) {
    fprintf(F, "wideData[%d]=%d\n", i, wideData[i]);
    }
  */

  /* count the lines in this exercise, and check the result
     against the screen length */
  for ( widep = wideData, lines_count = 0; *widep != ASCII_NULL; widep++ )
    {
      if ( *widep == ASCII_NL)
        lines_count++;
    }
  if ( DP_TOP_LINE + lines_count > LINES )
    fatal_error( _("data exceeds screen length"), line );

  /* if the last command was a tutorial, ensure we have
     the complete screen */
  if ( global_prior_command == C_TUTORIAL )
    {
      move( T_TOP_LINE, 0 ); clrtobot();
    }

  while (1)
    {
      /* display speed test pattern */
      linenum = DP_TOP_LINE;
      move( linenum, 0 ); clrtobot();
      for ( widep = wideData; *widep != ASCII_NULL; widep++ )
        {
          if ( *widep != ASCII_NL )
            {
              wideaddch(*widep);
            }
          else
            {
              /* emit return character */
              wideaddch(RETURN_CHARACTER);

              /* newline - move down the screen */
              linenum++;
              move( linenum, 0 );
            }
        }
      move( MESSAGE_LINE, COLS - utf8len( MODE_SPEEDTEST ) - 2 );
      ADDSTR_REV( MODE_SPEEDTEST );

      /* run the data */
      linenum = DP_TOP_LINE;
      move( linenum, 0 );
      for ( widep = wideData; *widep == ASCII_SPACE && *widep != ASCII_NULL; widep++ )
        wideaddch(*widep);

      for ( chars_typed = 0, errors = 0, error_sync = 0;
            *widep != ASCII_NULL; widep++ )
        {
          rc = getch_fl( (*widep != ASCII_NL) ? *widep : RETURN_CHARACTER );

          /* start timer on first char entered */
          if ( chars_typed == 0 )
            start_time = (long)time( NULL );
          chars_typed++;
          error_sync--;

          /* check for delete keys if not at line start or
             speed test start */
          if ( rc == KEY_BACKSPACE || rc == ASCII_BS || rc == ASCII_DEL )
            {
              /* just ignore deletes where it's impossible or hard */
              if ( widep > wideData && *(widep-1) != ASCII_NL && *(widep-1) != ASCII_TAB ) {
                /* back up one character */
                ADDCH( ASCII_BS ); widep--;
              }
              widep--;		/* defeat widep++ coming up */
              continue;
            }

          /* ESC is "give up"; ESC at beginning of exercise is "skip lesson"
             (this is handled outside the for loop) */
          if ( rc == ASCII_ESC )
            break;

          /* check that the character was correct */
          if ( rc == *widep || ( cl_args.word_processor_flag &&
				 rc == ASCII_SPACE && *widep == ASCII_NL ))
          { /* character is correct */
            if (*widep == ASCII_NL)
            {
                wideaddch(RETURN_CHARACTER);
            }
            else
            {
                wideaddch(rc);
            }
          }
          else 
            { /* character is incorrect */
              /* try to sync with typist behind */
              if ( error_sync >= 0 && widep > wideData && rc == *(widep-1) )
                {
                  widep--;
                  continue;
                }

              wideaddch_rev(*widep == ASCII_NL ? RETURN_CHARACTER : *widep);

              if ( ! cl_args.silent_flag ) {
                do_bell();
              }
              errors++;
              error_sync = 1;

              /* try to sync with typist ahead */
              if ( rc == *(widep+1) )
                {
                  ungetch( rc );
                  error_sync++;
                }
            }

          /* move screen location if newline */
          if ( *widep == ASCII_NL )
            {
              linenum++;
              move( linenum, 0 );
            }

          /* perform any other word processor like adjustments */
          if ( cl_args.word_processor_flag )
            {
              if ( rc == ASCII_SPACE )
                {
                  while ( *(widep+1) == ASCII_SPACE
                          && *(widep+1) != ASCII_NULL )
                    {
                      widep++; 
                      wideaddch(*widep);
                    }
                }
              else if ( rc == ASCII_NL )
                {
                  while ( ( *(widep+1) == ASCII_SPACE
                            || *(widep+1) == ASCII_NL )
                          && *(widep+1) != ASCII_NULL )
                    {
                      widep++;
                      wideaddch(*widep);
                      if ( *widep == ASCII_NL )
                        {
                          linenum++;
                          move( linenum, 0 );
                        }
                    }
                }
              else if ( isalpha(*widep) && *(widep+1) == ASCII_DASH
                        && *(widep+2) == ASCII_NL )
                {
                  widep++; 
                  wideaddch(*widep);
                  widep++;
                  wideaddch(*widep);
                  linenum++;
                  move( linenum, 0 );
                }
            }
        }


      /* ESC not at the beginning of the lesson: "give up" */
      if ( rc == ASCII_ESC && chars_typed != 1)
        continue; /* repeat */

      /* skip timings and don't check error-pct if exit was through ESC */
      if ( rc != ASCII_ESC )
        {
          /* display timings */
          end_time = (long)time( NULL );
          display_speed( chars_typed, end_time - start_time,
                         errors );

          /* check whether the error-percentage is too high (unless in s:) */
          if (drill_type != C_SPEEDTEST_PRACTICE_ONLY &&
              is_error_too_high(chars_typed, errors))
            {
              sprintf( message, ERROR_TOO_HIGH_MSG, global_error_max );
              wait_user (script, message, MODE_SPEEDTEST);

              /* check for F-command */
              if (global_on_failure_label != NULL)
                {
                  /* move to the label position in the file */
                  if (fseek(script, global_on_failure_label->offset, SEEK_SET )
                      == -1)
                    fatal_error( _("internal error: fseek"), NULL );
                  global_line_counter = global_on_failure_label->line_count;
                  /* tell the user about the misery :) */
                  sprintf(message,SKIPBACK_VIA_F_MSG,
                          global_on_failure_label->label);
                  /* reset value unless persistent */
                  if (!global_on_failure_label_persistent)
                    global_on_failure_label = NULL;
                  wait_user (script, message, MODE_SPEEDTEST);
                  seek_done = TRUE;
                  break;
                }

              continue;
            }
        }

      /* ask the user whether he/she wants to repeat or exit */
      if ( rc == ASCII_ESC && cl_args.no_skip_flag ) /* honor --no-skip */
        rc = do_query_repeat (script, FALSE);
      else
        rc = do_query_repeat (script, TRUE);
      if (rc == 'E') {
        seek_done = TRUE;
        break;
      }
      if (rc == 'N')
        break;

    }

  /* free the malloced memory */
  free( data );
  free( wideData );

  /* reset global_error_max */
  if (!global_error_max_persistent)
    global_error_max = cl_args.max_error_arg;

  /* buffer_command takes care of advancing `script' (and setting
     `line'), so we only do if seek_label had been called (in
     do_query_repeat or due to a failure and an F: command) */
  if (seek_done)
    get_script_line( script, line );
  global_prior_command = C_SPEEDTEST;
}


/*
 * clear the complete screen, maybe leaving a header behind
 */
static void do_clear (FILE *script, char *line)
{
  /* clear the complete screen */
  move( B_TOP_LINE , 0 ); clrtobot();

  banner (SCR_DATA (line));

  /* finally, get the next script command */
  get_script_line( script, line );
  global_prior_command = C_CLEAR;
}


/*
  go to a label - the flag is used to implement conditional goto's
*/
static void
do_goto( FILE *script, char *line, bool flag )
{
  char *line_iterator;

  /* reposition only if flag set - otherwise a noop */
  if ( flag )
    {
      /* remove trailing whitespace from line */
      line_iterator = line + strlen(line) - 1;
      while (line_iterator != line && isspace(*line_iterator))
	{
	  *line_iterator = '\0';
	  --line_iterator;
	}

      seek_label( script, SCR_DATA( line ), line );
    }
  get_script_line( script, line );
}


/*
  Ask the user whether he/she wants to repeat, continue or exit
  (this is used at the end of an exercise (drill/speedtest))
  The second argument is FALSE if skipping a lesson isn't allowed (--no-skip).
*/
static char
do_query_repeat ( FILE *script, bool allow_next )
{
  int resp;

  /* display the prompt */
  move( MESSAGE_LINE, 0 ); clrtoeol();
  move( MESSAGE_LINE, COLS - utf8len( MODE_QUERY ) - 2 );
  ADDSTR_REV( MODE_QUERY );
  move( MESSAGE_LINE, 0 );
  if (allow_next)
    ADDSTR_REV( REPEAT_NEXT_EXIT_MSG );
  else
    ADDSTR_REV( REPEAT_EXIT_MSG );

  /* wait for [RrNnEe] (or translation of these) */
  while (TRUE)
    {
      resp = getch_fl( ASCII_NULL );

      if (towideupper (resp) == 'R' ||
	  towideupper (resp) == RNE [0]) {
	resp = 'R';
	break;
      }
      if (allow_next && (towideupper (resp) == 'N' ||
			 towideupper (resp) == RNE [2])) {
	resp = 'N';
	break;
      }
      if (towideupper (resp) == 'E' || towideupper (resp) == RNE [4]) {
	if (do_query_simple (CONFIRM_EXIT_LESSON_MSG))
	  {
	    seek_label (script, fkey_bindings [11], NULL);
	    resp = 'E';
	    break;
	  }
	/* redisplay the prompt */
	move( MESSAGE_LINE, 0 ); clrtoeol();
	move( MESSAGE_LINE, COLS - utf8len( MODE_QUERY ) - 2 );
	ADDSTR_REV( MODE_QUERY );
	move( MESSAGE_LINE, 0 );
	if (allow_next)
	  ADDSTR_REV( REPEAT_NEXT_EXIT_MSG );
	else
	  ADDSTR_REV( REPEAT_EXIT_MSG );
      }
    }

  /* clear out the message line */
  move( MESSAGE_LINE, 0 ); clrtoeol();

  return (char)resp;
}


/*
  Same as do_query, but only used internally (doesn't set global_resp_flag,
  returns the value instead) and doesn't accept Fkeys.
  This is used to let the user confirm (E)xit.
*/
static bool
do_query_simple ( char *text )
{
  int resp;

  if (user_is_always_sure)
     return TRUE;

  /* display the prompt */
  move( MESSAGE_LINE, 0 ); clrtoeol();
  move( MESSAGE_LINE, COLS - strlen( MODE_QUERY ) - 2 );
  ADDSTR_REV( MODE_QUERY );
  move( MESSAGE_LINE, 0 );
  ADDSTR_REV( text );

  /* wait for Y/N or translation of Y/N */
  do
    {
      resp = getch_fl( ASCII_NULL );

      if (towideupper (resp) == 'Y' || towideupper (resp) == YN[0])
	resp = 0;
      else if (towideupper (resp) == 'N' || towideupper (resp) == YN[2])
	resp = -1;
    /* Some PDCURSES implementations return -1 when no key is pressed
       for a second or so.  So, unless resp is explicitly set to Y/N,
       don't exit! */
      else
	resp = 2;
    }  while (resp != 0 && resp != -1);

  /* clear out the message line */
  move( MESSAGE_LINE, 0 ); clrtoeol();

  return resp == 0 ? TRUE : FALSE;
}


/*
  get a Y/N response from the user: returns true if we just got the
  expected Y/N, false if exit was by a function key
*/
static bool
do_query( FILE *script, char *line )
{
  int	resp;			/* response character */
  int	fkey;			/* function key iterator */
  bool ret_code;

  /* display the prompt */
  move( MESSAGE_LINE, 0 ); clrtoeol();
  move( MESSAGE_LINE, COLS - strlen( MODE_QUERY ) - 2 );
  ADDSTR_REV( MODE_QUERY );
  move( MESSAGE_LINE, 0 );
  ADDSTR_REV( SCR_DATA( line ) );

  /* wait for a Y/N response, translation of Y/N or matching FKEY */
  while ( TRUE )
    {
      resp = getch_fl( ASCII_NULL );

      /* translate pseudo Fkeys into real ones if applicable
	 The pseudo keys are defined in array pfkeys and are also:
	 F1 - 1, F2 - 2, F3 - 3,.... F10 - 0, F11 - A, F12 - S */
      for ( fkey = 1; fkey <= NFKEYS; fkey++ )
	{
	  if ( resp == pfkeys[ fkey - 1 ] || (fkey<11 && resp == (fkey+'0'))
	       || (fkey==10 && (resp =='0'))
	       || (fkey==11 && (resp =='a' || resp=='A'))
	       || (fkey==12 && (resp =='s' || resp=='S')))
	    {
	      resp = KEY_F( fkey );
	      break;
	    }
	}

      /* search the key bindings for a matching key */
      for ( fkey = 1; fkey <= NFKEYS; fkey++ )
	{
	  if ( resp == KEY_F( fkey )
	       && fkey_bindings[ fkey - 1 ] != NULL )
	    {
	      seek_label( script, fkey_bindings[ fkey - 1 ],
			  NULL );
	      break;
	    }
	}
      if ( fkey <= NFKEYS ) {
	ret_code = FALSE;
	break;
      }

      /* no FKEY binding - check for Y or N */
      if ( towideupper( resp ) == QUERY_Y ||
	   towideupper( resp ) == YN[0] )
	{
	  ret_code = TRUE;
	  global_resp_flag = TRUE;
	  break;
	}
      if ( towideupper( resp ) == QUERY_N ||
	   towideupper( resp ) == YN[2] )
	{
	  ret_code = TRUE;
	  global_resp_flag = FALSE;
	  break;
	}
    }

  /* clear out the message line */
  move( MESSAGE_LINE, 0 ); clrtoeol();

  /* get the next command */
  get_script_line( script, line );

  /* tell the caller whether we got Y/N or a function key */
  return ( ret_code );
}

/*
  execute a E:-command: either "E: <value>%" (only applies to the next drill)
  or "E: <value>%*" (applies until the next E:-command)
*/
static void
do_error_max_set( FILE *script, char *line )
{
  char copy_of_line[MAX_SCR_LINE];
  char *data;
  bool star = FALSE;
  char *tail;
  double temp_value;

  /* we need to make a copy for a potential error-message */
  strcpy( copy_of_line, line );

  /* hide whitespace (and '*') */
  data = SCR_DATA( line ) + strlen( SCR_DATA( line ) ) - 1;
  while (data != SCR_DATA(line) && !star && (isspace( *data ) || *data == '*'))
    {
      if (*data == '*')
	star = TRUE;
      *data = '\0';
      --data;
    }
  data = SCR_DATA( line );
  while (isspace( *data ))
    ++data;

  /* set the state variables */
  global_error_max_persistent = star;
  if (strcmp( data, "default" ) == 0 || strcmp( data, "Default" ) == 0)
    global_error_max = cl_args.max_error_arg;
  else {
    /* value is not a special keyword */
    /* check for incorrect (not so readable) syntax */
    data = data + strlen( data ) - 1;
    if (*data != '%') {
      /* find out what's wrong */
      if (star && isspace( *data )) {
	/* find out whether `line' contains '%' */
	while (data != SCR_DATA( line ) && isspace( *data ))
	  {
	    *data = '\0';
	    --data;
	  }
	if (*data == '%')
	  /* xgettext: no-c-format */
	  fatal_error( _("'*' must immediately follow '%'"), copy_of_line );
	else
	  /* xgettext: no-c-format */
	  fatal_error( _("missing '%'"), copy_of_line );
      } else
	/* xgettext: no-c-format */
	fatal_error( _("missing '%'"), copy_of_line );
    }
    if (isspace( *(data - 1) ))
      /* xgettext: no-c-format */
      fatal_error( _("'%' must immediately follow value"), copy_of_line );
    /* remove '%' */
    *data = '\0';
    /* convert value: SCR_DATA(line) may contain whitespace at the
       beginning, but strtod ignores this */
    data = SCR_DATA( line );
    errno = 0;
    temp_value = (float)strtod( data, &tail );
    if (errno)
      fatal_error( _("overflow in do_error_max_set"), copy_of_line );
    /* TODO: if line="E:-1.0%", then tail will be ".0 "...
       if (*tail != '\0')
       fatal_error( _("can't parse value"), tail );*/
    /*
      If --error-max is specified (but *not* if the default value is used),
      an E:-command will only be applied if its level is more
      difficult (smaller) than the one specified via --error-max/-e
    */
    if (cl_args.max_error_given) {
      if (temp_value < cl_args.max_error_arg)
	global_error_max = temp_value;
      else
	global_error_max = cl_args.max_error_arg;
    } else
      global_error_max = temp_value;
  }

  /* sanity checks */
  if (global_error_max < 0.0 || global_error_max > 100.0)
    fatal_error( _("Invalid value for \"E:\" (out of range)"), copy_of_line );

  /* get the next command */
  get_script_line( script, line );
}

/*

*/
static void
do_on_failure_label_set( FILE *script, char *line )
{
  char copy_of_line[MAX_SCR_LINE];
  char *line_iterator;
  bool star = FALSE;
  int i;
  char	message[MAX_SCR_LINE];

  /* we need to make a copy for a potential error-message */
  strcpy( copy_of_line, line );

  /* remove trailing whitespace (and '*') */
  line_iterator = line + strlen( line ) - 1;
  while (line_iterator != line && !star &&
	 (isspace( *line_iterator ) || *line_iterator == '*'))
    {
      if (*line_iterator == '*')
	star = TRUE;
      *line_iterator = '\0';
      --line_iterator;
    }

  global_on_failure_label_persistent = star;

  /* check for special value "NULL" */
  if (strcmp(SCR_DATA(line), "NULL") == 0)
    global_on_failure_label = NULL;
  else
    {
      /* find the right hash list for the label */
      i = hash_label( SCR_DATA(line) );

      /* search the linked list for the label */
      for ( global_on_failure_label = global_label_list[i];
	    global_on_failure_label != NULL;
	    global_on_failure_label = global_on_failure_label->next )
	{
	  /* see if this is our label */
	  if ( strcmp( global_on_failure_label->label, SCR_DATA(line) ) == 0 )
	    break;
	}

      /* see if the label was not found in the file */
      if ( global_on_failure_label == NULL )
	{
	  sprintf( message, _("label '%s' not found"), SCR_DATA(line) );
	  fatal_error( message, copy_of_line );
	}
    }

  /* get the next command */
  get_script_line( script, line );
}

/*
  execute the directives in the script file
*/
static void
parse_file( FILE *script, char *label ) {

  char	line[MAX_SCR_LINE];		/* line buffer */
  char	command;			/* current command */

  /* if label given then start running there */
  if ( label != NULL )
    {
      /* find the label we want to start at */
      seek_label( script, label, NULL );
    }
  else
    {
      /* start at the very beginning (a very good place to start) */
      rewind( script );
      global_line_counter = 0;
    }
  get_script_line( script, line );

  /* just handle lines until the end of the file */
  while( ! feof( script ))
    {
      command = SCR_COMMAND( line );
      switch( command )
	{
	case C_TUTORIAL:
	  do_tutorial( script, line ); break;
	case C_INSTRUCTION:
	  do_instruction( script, line ); break;
	case C_CLEAR:	do_clear( script, line ); break;
	case C_GOTO:	do_goto( script, line, TRUE ); break;
	case C_EXIT:	do_exit( script ); break;
	case C_QUERY:	do_query( script, line ); break;
	case C_YGOTO:	do_goto( script, line, global_resp_flag );
	  break;
	case C_NGOTO:	do_goto( script, line, !global_resp_flag );
	  break;
	case C_DRILL:
	case C_DRILL_PRACTICE_ONLY:
	  do_drill( script, line ); break;
	case C_SPEEDTEST:
	case C_SPEEDTEST_PRACTICE_ONLY:
	  do_speedtest( script, line ); break;
	case C_KEYBIND:	do_keybind( script, line ); break;

	case C_LABEL:
	   __update_last_label (SCR_DATA (line));
	   get_script_line (script, line);
	   break;
	case C_ERROR_MAX_SET: do_error_max_set( script, line ); break;
	case C_ON_FAILURE_SET: do_on_failure_label_set( script, line ); break;
	case C_MENU: do_menu (script, line); break;
	default:
	  fatal_error( _("unknown command"), line );
	  break;
	}
    }
}

/*
  Parse command line arguments and config file
*/
static void
parse_cmdline_and_config( int argc, char **argv )
{
    // parse command line
    if( cmdline_parser( argc, argv, &cl_args ) != 0 )
	exit( 1 );

    // parse config file
    struct cmdline_parser_params params;
    cmdline_parser_params_init( &params );
    params.initialize = 0;
    params.check_required = 0;
    if( cmdline_parser_config_file( ".gtypistrc", &cl_args, &params ) != 0 )
	exit( 1 );

    // check there is at most one script specified
    if( cl_args.inputs_num > 1 ) {
      fprintf( stderr, _( "Try '%s --help' for more information.\n" ), argv0 );
      exit( 1 );
    }

    // check max-error is valid
    if( cl_args.max_error_arg <= 0 || cl_args.max_error_arg > 100 ) {
	fprintf( stderr, _( "%s: invalid error-max value\n" ), argv0 );
	exit( 1 );
    }

    // check curs-flash is valid
    if( cl_args.curs_flash_arg < 0 || cl_args.curs_flash_arg > 512 ) {
	fprintf( stderr, _( "%s: invalid curs-flash value\n" ), argv0 );
	exit( 1 );
    }

    // parse and check colours
    if( sscanf( cl_args.colours_arg, "%d,%d",
		&cl_fgcolour, &cl_bgcolour ) != 2 ||
	cl_fgcolour < 0 || cl_fgcolour >= NUM_COLOURS ||
	cl_bgcolour < 0 || cl_bgcolour >= NUM_COLOURS )
    {
	fprintf( stderr, _( "%s: invalid colours value\n" ), argv0 );
	exit( 1 );
    }

    // parse and check banner-colors
    if( sscanf( cl_args.banner_colors_arg, "%d,%d,%d,%d",
		&cl_banner_fg_colour, &cl_banner_bg_colour,
		&cl_prog_name_colour, &cl_prog_version_colour) != 4 ||
	cl_banner_bg_colour < 0 || cl_banner_bg_colour >= NUM_COLOURS ||
	cl_banner_fg_colour < 0 || cl_banner_bg_colour >= NUM_COLOURS ||
	cl_prog_version_colour < 0 || cl_prog_version_colour >= NUM_COLOURS ||
	cl_prog_name_colour < 0 || cl_prog_name_colour >= NUM_COLOURS )
    {
	fprintf( stderr, _( "%s: invalid banner-colours value\n" ), optarg );
	exit( 1 );
    }
}

/*
  signal handler to stop curses stuff on intr
*/
static void
catcher( int signal ) {

  /* unravel colours and curses, clean up and exit */
  if ( cl_args.colours_given && has_colors() )
    wbkgdset( stdscr, 0 );
  clear(); refresh(); endwin();
  printf("\n");
  exit( 1 );
}

/*
  open a script file
 */
FILE *open_script( const char *filename )
{
  FILE *script;

#ifdef MINGW
  /* MinGW's ftell doesn't work properly for absolute file positions in
     text mode, so open in binary mode instead. */
  script = fopen( filename, "rb" );
#else
  script = fopen( filename, "r" );
#endif

  /* record script filename */
  if( script != NULL )
    global_script_filename = strdup( filename );

  return script;
}

/*
  main routine
*/
int main( int argc, char **argv )
{
  WINDOW	*scr;			/* curses window */
  FILE	*script;			/* script file handle */
  char	*p, filepath[FILENAME_MAX];	/* file paths */
  char	script_file[FILENAME_MAX];	/* more file paths */

  /* get our program name */
  argv0 = argv[0] + strlen( argv[0] );
  while ( argv0 > argv[0] && *argv0 != '/' )
    argv0--;
  if ( *argv0 == '/' ) argv0++;

  /* Internationalization */
#if defined(ENABLE_NLS) && defined(LC_ALL)
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  /* make gettext always return strings as UTF-8
     => this makes programming easier because now _all_ strings
     (from gettext and from script file) are encoded as UTF8!
   */
  bind_textdomain_codeset(PACKAGE, "utf-8"); 
  textdomain (PACKAGE);
#endif

#ifdef MINGW
  locale_encoding = "UTF-8";
#else
  locale_encoding = nl_langinfo(CODESET);
#endif
  isUTF8Locale = strcasecmp(locale_encoding, "UTF-8") == 0 ||
      strcasecmp(locale_encoding, "UTF8") == 0;
  /* printf("encoding is %s, UTF8=%d\n", locale_encoding, isUTF8Locale); */

  COPYRIGHT=
    _("Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 Simon Baldwin.\n"
      "Copyright (C) 2003, 2004, 2008, 2011, 2012 GNU Typist Development Team.\n"
      "This program comes with ABSOLUTELY NO WARRANTY; for details\n"
      "please see the file 'COPYING' supplied with the source code.\n"
      "This is free software, and you are welcome to redistribute it\n"
      "under certain conditions; again, see 'COPYING' for details.\n"
      "This program is released under the GNU General Public License.");
  /* this string is displayed in the mode-line when in a tutorial */
  MODE_TUTORIAL=_(" Tutorial ");
  /* this string is displayed in the mode-line when in a query */
  MODE_QUERY=_("  Query   ");
  /* this string is displayed in the mode-line when running a drill */
  MODE_DRILL=_("  Drill   ");
  /* this string is displayed in the mode-line when running a speedtest */
  MODE_SPEEDTEST=_("Speed test");
  WAIT_MESSAGE=
	  _(" Press RETURN or SPACE to continue, ESC to return to the menu ");
  /* this message is displayed when the user has failed in a [DS]: drill */
  ERROR_TOO_HIGH_MSG=
    _(" Your error-rate is too high. You have to achieve %.1f%%. ");
  /* this message is displayed when the user has failed in a [DS]: drill,
     and an F:<LABEL> ("on failure label") is in effect */
  SKIPBACK_VIA_F_MSG=
    _(" You failed this test, so you need to go back to %s. ");
  /* this is used for repeat-queries. you can translate the keys as well
     (if you translate msgid "R/N/E" accordingly) */
  REPEAT_NEXT_EXIT_MSG=
	_(" Press R to repeat, N for next exercise or E to exit ");
  /* this is used for repeat-queries with --no-skip. you can translate
     the keys as well (if you translate msgid "R/N/E" accordingly) */
  REPEAT_EXIT_MSG=
	_(" Press R to repeat or E to exit ");
  /* This is used make the user confirm (E)xit in REPEAT_NEXT_EXIT_MSG */
  CONFIRM_EXIT_LESSON_MSG=
    _(" Are you sure you want to exit this lesson? [Y/N] ");
  /* This message is displayed if the user tries to skip a lesson
     (ESC ESC) and --no-skip is specified */
  NO_SKIP_MSG=
    _(" Sorry, gtypist is configured to forbid skipping exercises. ");
  /* this is used to translate the keys for Y/N-queries. Must be two
     uppercase letters separated by '/'. Y/N will still be accepted as
     well. Note that the messages (prompts) themselves cannot be
     translated because they are read from the script-file. */
  YN = convertFromUTF8(_("Y/N"));
  if (wcslen(YN) != 3 || YN[1] != '/' || !iswideupper(YN[0]) || !iswideupper(YN[2]))
    {
      fprintf( stderr,
	       "%s: i18n problem: invalid value for msgid \"Y/N\" (3 uppercase UTF-8 chars?): %ls\n",
	       argv0, YN );
      exit( 1 );
    }
  /* this is used to translate the keys for Repeat/Next/Exit
     queries. Must be three uppercase letters separated by slashes. */
  RNE = convertFromUTF8(_("R/N/E"));
  if (wcslen(RNE) != 5 ||
      !iswideupper(RNE[0]) || RNE[1] != '/' ||
      !iswideupper(RNE[2]) || RNE[3] != '/' ||
      !iswideupper(RNE[4]))
    {
      fprintf( stderr,
	       "%s: i18n problem: invalid value for msgid \"R/N/E\" (5 uppercase UTF-8 chars?): %ls\n",
	       argv0, RNE );
      exit( 1 );
    }

  /* parse command line argments and config file */
  parse_cmdline_and_config( argc, argv );

  /* figure out what script file to use */
  if ( cl_args.inputs_num == 1 )
    {
      /* try and open scipr file from command line */
      strcpy( script_file, cl_args.inputs[ 0 ] );
      script = open_script( script_file );

      /* we failed, so check for script in GTYPIST_PATH */
      if( !script && getenv( "GTYPIST_PATH" ) )
        {
          for( p = strtok( getenv( "GTYPIST_PATH" ), ":" );
	       p != NULL; p = strtok( NULL, ":" ) )
	    {
	      strcpy( filepath, p );
	      strcat( filepath, "/" );
	      strcat( filepath, script_file );
	      script = open_script( filepath );
	      if( script )
		break;
	    }
	}

      /* we failed, so try to find script in DATADIR */
      if( !script )
        {
          strcpy( filepath, DATADIR );
          strcat( filepath, "/" );
          strcat( filepath, script_file );
          script = open_script( filepath );
	}
    }
  else
    {
      /* open default script */
      sprintf( script_file, "%s/%s", DATADIR, DEFAULT_SCRIPT );
      script = open_script( script_file );
    }

  /* check to make sure we open a script */
  if( !script )
    {
      fprintf( stderr, "%s: %s %s\n",
	       argv0, _("can't find or open file"), script_file );
      exit( 1 );
    }

  /* reset global_error_max */
  global_error_max = cl_args.max_error_arg;

  /* check for user home directory */
#ifdef MINGW
  global_home_env = "APPDATA";
#else
  global_home_env = "HOME";
#endif
  if( !getenv( global_home_env ) || !strlen( getenv( global_home_env ) ) )
    {
      fprintf( stderr, _("%s: %s environment variable not set\n"), \
	  argv0, global_home_env );
      exit( 1 );
    }

  /* prepare for curses stuff, and set up a signal handler
     to undo curses if we get interrupted */
  scr = initscr();
  signal( SIGINT, catcher );
  signal( SIGTERM, catcher );
#ifndef MINGW
  signal( SIGHUP, catcher );
  signal( SIGQUIT, catcher );
#ifndef DJGPP
  signal( SIGCHLD, catcher );
#endif
  signal( SIGPIPE, catcher );
#endif
  clear(); refresh(); typeahead( -1 );
  keypad( scr, TRUE ); noecho(); curs_set( 0 ); raw();

  // Quick hack to get rid of the escape delays
#ifdef __NCURSES_H
  ESCDELAY = 1;
#endif

  /* set up colour pairs if possible */
  if (has_colors ())
  {
     start_color ();

     init_pair (C_NORMAL,
		 colour_array [cl_fgcolour],
		 colour_array [cl_bgcolour]);
     wbkgdset (stdscr, COLOR_PAIR (C_NORMAL));

     init_pair (C_BANNER,
		     colour_array [cl_banner_fg_colour],
		     colour_array [cl_banner_bg_colour]);
     init_pair (C_PROG_NAME,
		     colour_array [cl_banner_fg_colour],
		     colour_array [cl_prog_name_colour]);
     init_pair (C_PROG_VERSION,
		     colour_array [cl_banner_fg_colour],
		     colour_array [cl_prog_version_colour]);
     init_pair (C_MENU_TITLE,
		     colour_array [cl_fgcolour],
		     colour_array [cl_bgcolour]);
  }

  /* put up the top line banner */
  clear();
  banner (_("Loading the script..."));

  if (!do_beginner_infoview())
  {
      do_exit(script);
      return 0;
  }

  check_script_file_with_current_encoding(script);

  /* index all the labels in the file */
  build_label_index( script );

  /* run the input file */
  parse_file( script, cl_args.start_label_arg );
  do_exit( script );

  /* for lint... */
  return( 0 );
}

void do_bell() {
#ifndef MINGW
  putchar( ASCII_BELL );
  fflush( stdout );
#endif
}

bool get_best_speed( const char *script_filename,
		     const char *excersise_label, double *adjusted_cpm )
{
  FILE *blfile;				/* bestlog file */
  char *filename;			/* bestlog filename */
  char *search;				/* string to match in bestlog */
  char line[FILENAME_MAX];		/* single line from bestlog */
  int search_len;			/* length of search string */
  bool found = FALSE;			/* did we find it? */
  int a;
  char *fixed_script_filename;		/* fixed-up script filename */
  char *p;

  /* calculate filename */
  filename = (char *)malloc( strlen( getenv( global_home_env ) ) +
                             strlen( BESTLOG_FILENAME ) + 2 );
  if( filename == NULL )
    {
       perror( "malloc" );
       fatal_error( _( "internal error: malloc" ), NULL );
    }
  sprintf( filename, "%s/%s", getenv( global_home_env ), BESTLOG_FILENAME );

  /* open best speeds file */
  blfile = fopen( filename, "r" );
  if( blfile == NULL )
    {
      free( filename );
      return FALSE;
    }

  /* fix-up script filename */
  fixed_script_filename = strdup( script_filename );
  if( fixed_script_filename == NULL )
    {
       perror( "malloc" );
       fatal_error( _( "internal error: malloc" ), NULL );
    }
  p = fixed_script_filename;
  while( *p != '\0' )
    {
      if( *p == ' ' )
	*p = '+';
      p++;
    }

  /* construct search string */
  search_len = strlen( script_filename ) + strlen( excersise_label ) + 3;
  search = (char *)malloc( search_len + 1 );
  if( search == NULL )
    {
       perror( "malloc" );
       fatal_error( _( "internal error: malloc" ), NULL );
    }
  sprintf( search, " %s:%s ", fixed_script_filename, excersise_label );

  /* search for lines that match and use data from the last one */
  while( fgets( line, FILENAME_MAX, blfile ) )
    {
      /* check that there are at least 19 chars (yyyy-mm-dd hh:mm:ss) */
      for( a = 0; a < 19; a++ )
	if( line[ a ] == '\0' )
	  continue;

      /* look for search string and try to obtain a speed */
      if( !strncmp( search, line + 19, search_len ) &&
	  sscanf( line + 19 + search_len, "%lg", adjusted_cpm ) == 1 )
	{
	  found = TRUE;
	}
    }

  /* cleanup and return */
  free( search );
  free( filename );
  free( fixed_script_filename );
  fclose( blfile );
  return found;
}

void put_best_speed( const char *script_filename,
		     const char *excersise_label, double adjusted_cpm )
{
  FILE *blfile;				/* bestlog file */
  char *filename;			/* bestlog filename */
  char *fixed_script_filename;		/* fixed-up script filename */
  char *p;

  /* calculate filename */
  filename = (char *)malloc( strlen( getenv( global_home_env ) ) +
                             strlen( BESTLOG_FILENAME ) + 2 );
  if( filename == NULL )
    {
       perror( "malloc" );
       fatal_error( _( "internal error: malloc" ), NULL );
    }
  sprintf( filename, "%s/%s", getenv( global_home_env ), BESTLOG_FILENAME );

  /* open best speeds files */
  blfile = fopen( filename, "a" );
  if( blfile == NULL )
    {
       perror( "fopen" );
       fatal_error( _("internal error: fopen" ), NULL );
    }

  /* fix-up script filename */
  fixed_script_filename = strdup( script_filename );
  if( fixed_script_filename == NULL )
    {
       perror( "malloc" );
       fatal_error( _( "internal error: malloc" ), NULL );
    }
  p = fixed_script_filename;
  while( *p != '\0' )
    {
      if( *p == ' ' )
	*p = '+';
      p++;
    }

  /* get time */
  time_t nowts = time( NULL );
  struct tm *now = localtime( &nowts );

  /* append new score */
  fprintf( blfile, "%04d-%02d-%02d %02d:%02d:%02d %s:%s %g\n",
	   now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour,
	   now->tm_min, now->tm_sec, fixed_script_filename, excersise_label,
	   adjusted_cpm );

  /* cleanup */
  free( filename );
  free( fixed_script_filename );
  fclose( blfile );
}

/*
  Local Variables:
  tab-width: 8
  End:
*/
