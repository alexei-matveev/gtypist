
/*
 * GNU Typist  - interactive typing tutor program for UNIX systems
 * Copyright (C) 1998,2001  Simon Baldwin (simonb@sco.com)
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
 *
 */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <curses.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include "config.h"

/* Internationalization */

#ifndef ENABLE_NLS

#define _(String) (String)

#else /* not ENABLE_NLS */

#include <libintl.h>
#define _(String) gettext (String)

#endif /* not ENABLE_NLS */

/* VERSION and PACKAGE defined in config.h */

char *COPYRIGHT;

/* a definition of a boolean type */
#define bool			int

/* limits on line lengths from the script file and screen */
#define	MAX_SCR_LINE		1024
#define	MIN_SCR_LINE		2
#define	MAX_WIN_LINE		128

/* some screen postions */
#define	MESSAGE_LINE		(LINES - 1)
#define B_TOP_LINE		0
#define T_TOP_LINE		(B_TOP_LINE + 1)
#define I_TOP_LINE		(T_TOP_LINE)
#define DP_TOP_LINE		(I_TOP_LINE + 2)
#define	SPEED_LINE		(LINES - 5)

/* things to help parse the input file */
#define	SCR_COMMAND(X)		(X[0])
#define	SCR_SEP(X)		(X[1])
#define	SCR_DATA(X)		(&X[2])
#define	C_COMMENT		'#'
#define	C_ALT_COMMENT		'!'
#define	C_SEP			':'
#define	C_CONT			' '
#define	C_LABEL			'*'
#define	C_TUTORIAL		'T'
#define	C_INSTRUCTION		'I'
#define	C_CLEAR			'B'
#define	C_GOTO			'G'
#define	C_EXIT			'X'
#define	C_QUERY			'Q'
#define	C_YGOTO			'Y'
#define	C_NGOTO			'N'
#define	C_DRILL			'D'
#define C_DRILL_PRACTICE_ONLY   'd'
#define	C_SPEEDTEST		'S'
#define	C_SPEEDTEST_PRACTICE_ONLY 's'
#define	C_KEYBIND		'K'
#define C_ERROR_MAX_SET         'E'
#define C_ON_FAILURE_SET        'F'

/* mode indicator strings */
char *MODE_TUTORIAL;
char *MODE_QUERY;
char *MODE_DRILL;
char *MODE_SPEEDTEST;

/* yes/no responses and miscellanea */
#define	QUERY_Y			'Y'
#define	QUERY_N			'N'
#define	DRILL_CH_ERR		'^'
#define	DRILL_NL_ERR		'<'
/* this is needed in wait_user
   DJGPP defines '\n' as 0x0A, but pdcurses 2.4 returns 0x0D (because
   raw() is called) */
#ifndef DJGPP
#define ASCII_ENTER             '\n'
#else
#define ASCII_ENTER             0x0D
#endif
#define	ASCII_NL		'\n'
#define	ASCII_NULL		'\0'
#define	ASCII_ESC		27
#define	ASCII_BELL		7
#define	ASCII_SPACE		' '
#define	ASCII_DASH		'-'
#define	ASCII_TAB		'\t'
#define	ASCII_BS		8
#define	ASCII_DEL		127
#define	STRING_NL		"\n"
char *WAIT_MESSAGE;
char *ERROR_TOO_HIGH_MSG;
char *SKIPBACK_VIA_F_MSG;
char *REPEAT_NEXT_EXIT_MSG;
char *REPEAT_EXIT_MSG;
char *CONFIRM_EXIT_LESSON_MSG;
char *NO_SKIP_MSG;
char *SPEED_RAW;
char *SPEED_ADJ;
char *SPEED_PCT_ERROR;
char *YN;
char *RNE;

#ifndef PACKAGE_DATA_DIR
#define PACKAGE_DATA_DIR "."
#endif

#define	DEFAULT_SCRIPT      "gtypist.typ"


/* some colour definitions */
static short	colour_array[] = {
  COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
  COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE };
#define	NUM_COLOURS		(sizeof( colour_array ) / sizeof( short ))
#define	C_NORMAL		1		/* normal colour pair */

/* shortcuts for reverse/normal mode strings */
#define	ADDSTR_REV(X)		do { attron( A_REVERSE ); addstr( X ); \
				attroff( A_REVERSE ); } while ( 0 )
#define	ADDSTR(X)		addstr( X )	/* for symmetry! */
/* the casts are necessary to handle 8 bit characters correctly */
#define	ADDCH_REV(X)		do { attron( A_REVERSE ); \
                                addch( (unsigned char)X ); \
				attroff( A_REVERSE ); } while ( 0 )
#define	ADDCH(X)		addch( (unsigned char) X )

/* command line options/values */
static bool     cl_error_max_specified = FALSE; /* is --error-max specified? */
static float	cl_default_error_max = 3.0;	/* maximum error percentage */
static bool	cl_notimer = FALSE;		/* no timings in drills */
static bool	cl_term_cursor = FALSE;		/* don't do s/w cursor */
static int	cl_curs_flash = 10;		/* cursor flash period */
static bool	cl_silent = FALSE;		/* no beep on errors */
static char	*cl_start_label = NULL;		/* initial lesson start point*/
static bool	cl_colour = FALSE;		/* set if -c given */
static int	cl_fgcolour = 0;		/* fg colour */
static int	cl_bgcolour = 0;		/* bg colour */
static bool	cl_wp_emu = FALSE;		/* do wp-like stuff */
static bool     cl_no_skip = FALSE;             /* forbid the user to */

/* a few global variables */
static char	*argv0 = NULL;
static bool	global_resp_flag = TRUE;
static int	global_line_counter = 0;
static char	global_prior_command = C_CONT;

static float    global_error_max = -1.0;
static bool     global_error_max_persistent = FALSE;

static struct label_entry *global_on_failure_label = NULL;
static bool     global_on_failure_label_persistent = FALSE;

/* a global area for label indexing - singly linked lists, hashed */
#define	NLHASH			32		/* num hash lists */
struct label_entry {
  char		*label;			/* label string */
  long		offset;			/* offset into file */
  int		line_count;		/* line number in script */
  struct label_entry	*next;		/* pointer to next element */
};
static struct label_entry	*global_label_list[NLHASH];
/* list head */

/* a global area for associating function keys with labels */
#define NFKEYS			12		/* num of function keys */
static	char	*fkey_bindings[ NFKEYS ] =
  { NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL };
/* table of pseudo-function keys, to allow ^Q to double as Fkey1, etc */
#define	CTRL_OFFSET		0100		/* ctrl keys are 'X' - 0100 */
static	char	pfkeys[ NFKEYS ] =
  { 'Q'-CTRL_OFFSET, 'W'-CTRL_OFFSET, 'E'-CTRL_OFFSET, 'R'-CTRL_OFFSET,
    'T'-CTRL_OFFSET, 'Y'-CTRL_OFFSET, 'U'-CTRL_OFFSET, 'I'-CTRL_OFFSET,
    'O'-CTRL_OFFSET, 'P'-CTRL_OFFSET, 'A'-CTRL_OFFSET, 'S'-CTRL_OFFSET };


#define MAX(A,B) ((A)<(B)?(B):(A))


/* prototypes */

static int getch_fl( char cursor_char );
static void fatal_error( char *message, char *line );
static void get_script_line( FILE *script, char *line );
static int hash_label( char *label );
static void index_labels( FILE *script );
static void seek_label( FILE *script, char *label, char *ref_line );
static void wait_user( char *message, char *mode );
static void display_speed( int total_chars, long elapsed_time, int errcount );
static void do_keybind( FILE *script, char *line );
static void do_tutorial( FILE *script, char *line );
static void do_instruction( FILE *script, char *line );
static char *buffer_command( FILE *script, char *line );
static void do_drill( FILE *script, char *line );
static void do_speedtest( FILE *script, char *line );
static void do_clear( FILE *script, char *line );
static void do_goto( FILE *script, char *line, bool flag );
static void do_exit( FILE *script );
static char do_query_repeat( FILE *script, bool allow_next );
static bool do_query_simple( char *text );
static bool do_query( FILE *script, char *line );
static void do_error_max_set( FILE *script, char *line );
static void do_on_failure_label_set( FILE *script, char *line );
static void parse_file( FILE *script, char *label );
static void indent_to( int n );
static void print_usage_item( char *op, char *lop, char *help, 
			      int col_op, int col_lop, int col_help,
			      int last_col );
static void print_help();
static void parse_cmdline( int argc, char **argv );
static void catcher( int signal );

/*
  getch() that does a flashing cursor; some xterms seem to be
  unwilling to do A_BLINK, however, the program needs some
  way to separate out the inverse char cursor from the inverse
  char mistyping indications.  And to complicate things, some
  xterms seem not make the cursor invisible either.
*/
static int 
getch_fl( char cursor_char ) 
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
      cbreak(); return_char = getch();
      move( y, x );
    }
  else 
    {
      /* produce a flashing cursor, or not, as requested */
      if ( ! cl_term_cursor ) {
	/* go for the flashing block here */
	ADDCH_REV( cursor_char );
	curs_set( 0 ); refresh();
	move( LINES - 1, COLS - 1 );
	if ( ( cl_curs_flash / 2 ) > 0 ) 
	  {
	    halfdelay( cl_curs_flash / 2 );
	    while ( (return_char = getch()) == ERR ) 
	      {
		move( y, x );
		if ( alternate )
		  ADDCH_REV( cursor_char );
		else
		  ADDCH( cursor_char );
		move( LINES - 1, COLS - 1 );
		alternate = !alternate;
	      }
	  }
	else 
	  {
	    cbreak(); return_char = getch();
	  }
	move( y, x );
	ADDCH( cursor_char );
	move( y, x );
      }
      else 
	{
	  /* just use the terminal's cursor - this is easy */
	  curs_set( 1 ); refresh();
	  cbreak(); return_char = getch();
	  curs_set( 0 ); refresh();
	}
    }
  
  /* return what key was pressed */
  return ( return_char );
}


/*
  handle fatal errors (pretty much any error) by dropping out
  of curses etc, and printing a complaint
  message that is already translated to the appropriate language 
*/
static void 
fatal_error( char *message, char *line ) {

  /* stop curses */
  if ( cl_colour && has_colors() )
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


/*
  get the next non-comment, non-blank line from the script file
  and check its basic format
*/
static void 
get_script_line( FILE *script, char *line ) {

  /* get lines until not empty/comment, or eof found */
  fgets(line, MAX_SCR_LINE, script);
  global_line_counter++;
  while ( ! feof( script ) && 
	  ( strcmp( line, "\n" ) == 0 ||
	    SCR_COMMAND( line ) == C_COMMENT ||
	    SCR_COMMAND( line ) == C_ALT_COMMENT )) 
    {
      fgets(line, MAX_SCR_LINE, script);
      global_line_counter++;
    }

  /* if a line was read then check it */
  if ( ! feof( script )) 
    {
      if ( line[strlen( line ) - 1] == ASCII_NL )
	line[strlen( line ) - 1] = ASCII_NULL;
      if ( strlen( line ) < MIN_SCR_LINE )
	fatal_error( _("data shortage"), line );
      if ( SCR_SEP( line ) != C_SEP )
	fatal_error( _("missing ':'"), line );
      if ( SCR_COMMAND( line ) != C_LABEL 
	   && SCR_COMMAND( line ) != C_GOTO 
	   && SCR_COMMAND( line ) != C_YGOTO
	   && SCR_COMMAND( line ) != C_NGOTO
	   && strlen( SCR_DATA( line )) > COLS )
	fatal_error( _("line too long for screen"), line );
    }
}


/*
  label hashing function - returns an index for the lists
*/
static int 
hash_label( char *label ) {
  char	*p;				/* pointer through string */
  int	csum = 0;			/* sum of characters */
  
  /* hash by summing the characters and taking modulo of the
     number of hash lists defined */
  for ( p = label; *p != ASCII_NULL; p++ )
    csum += *p;
  return ( csum % NLHASH );
}


/*
  go through the file and index all the labels we can find
*/
static void index_labels( FILE *script ) {
  char line[MAX_SCR_LINE];
  char *line_iterator;
  struct label_entry	*new_label = NULL;	/* new label entry */
  struct label_entry	*list_tail[NLHASH];	/* tail tracking */
  int	hash;					/* hash index */
  struct label_entry	*check_label;		/* pointer thru list */
  
  /* start at the file's beginning */
  rewind( script );
  global_line_counter = 0;
  
  /* initialize the hash lists */
  for ( hash = 0; hash < NLHASH; hash++ ) 
    {
      global_label_list[ hash ] = NULL;
      list_tail[ hash ] = NULL;
    }
  
  /* read until we get to eof */
  get_script_line( script, line );
  while (! feof( script )) 
    {
      
      /* see if this is a label line */
      if ( SCR_COMMAND( line ) == C_LABEL ) 
	{
	  
	  /* note this label's position in the table;
	     first, create a new list entry */
	  new_label = malloc( sizeof(struct label_entry) );
	  if ( new_label == NULL )
	    fatal_error( _("internal error: malloc"), line );
	  
	  /* remove trailing whitespace from line */
	  line_iterator = line + strlen(line) - 1;
	  while (line_iterator != line && isspace(*line_iterator))
	    {
	      *line_iterator = '\0';
	      --line_iterator;
	    }

	  /* make some space for the label string */
	  new_label->label =
	    malloc( strlen( SCR_DATA( line )) + 1 );
	  if ( new_label->label == NULL )
	    fatal_error( _("internal error: malloc"), line );
	  
	  /* copy the data into the new structure */
	  strcpy( new_label->label, SCR_DATA( line ));
	  new_label->offset = ftell( script );
	  new_label->line_count = global_line_counter;
	  new_label->next = NULL;
	  
	  /* find the right hash list for the label */
	  hash = hash_label( SCR_DATA( line ));
	  
	  /* search the linked list for the label, to
	     see if it's already there - nice to check */
	  for ( check_label = global_label_list[ hash ];
		check_label != NULL;
		check_label = check_label->next ) 
	    {
	      if ( strcmp( check_label->label, SCR_DATA( line )) == 0 )
		fatal_error( _("label redefinition"), line );
	    }
	  
	  /* link everything together */
	  if ( list_tail[ hash ] == NULL )
	    global_label_list[ hash ] = new_label;
	  else
	    list_tail[ hash ]->next = new_label;
	  list_tail[ hash ] = new_label;
	}
      
      /* get the next script line */
      get_script_line( script, line );
    }
}


/*
  search out a label from the file, and set the file pointer to
  that location
*/
static void 
seek_label( FILE *script, char *label, char *ref_line ) {
  struct label_entry	*check_label;	/* pointer through list */
  int	hash;				/* hash index */
  char	err[MAX_SCR_LINE];		/* error message string */
  
  /* find the right hash list for the label */
  hash = hash_label( label );
  
  /* search the linked list for the label */
  for ( check_label = global_label_list[ hash ]; check_label != NULL;
	check_label = check_label->next ) 
    {
      
      /* see if this is our label */
      if ( strcmp( check_label->label, label ) == 0 )
	break;
    }

  /* see if the label was not found in the file */
  if ( check_label == NULL ) 
    {
      sprintf( err, _("label '%s' not found"), label );
      fatal_error( err, ref_line );
    }

  /* move to the label position in the file */
  if ( fseek( script, check_label->offset, SEEK_SET ) == -1 )
    fatal_error( _("internal error: fseek"), ref_line );
  global_line_counter = check_label->line_count;
}


/*
  wait for a nod from the user before continuing
*/
static void
wait_user( char *message, char *mode )
{
  int	resp;			/* response character */

  /* move to the message line print a prompt */
  move( MESSAGE_LINE, 0 ); clrtoeol();
  move( MESSAGE_LINE, COLS - strlen( mode ) - 2 );
  ADDSTR_REV( mode );
  move( MESSAGE_LINE, 0 );
  ADDSTR_REV( message );

  do {
    resp = getch_fl( ASCII_NULL );
  } while (resp != ASCII_ENTER);

  /* clear the message line */
  move( MESSAGE_LINE, 0 ); clrtoeol();
}


/*
  display speed and accuracy from a drill or speed test
*/
static void display_speed( int total_chars, long elapsed_time, int errcount ) {
  double	test_time;			/* time in minutes */
  double	words;				/* effective words typed */
  double	speed, adjusted_speed;		/* speeds in wpm */
  char	message[MAX_WIN_LINE]; 		        /* buffer */

  /* calculate the speeds */
  test_time = (double)elapsed_time / (double)60.0;
  words = (double)total_chars / (double)5.0;
  if ( elapsed_time > 0 ) 
    {
      speed = words / test_time;
      speed = ( speed < 999.99 ) ? speed : 999.99;
      adjusted_speed = ( words - errcount ) / test_time;
      adjusted_speed = ( adjusted_speed < 999.99 )
	? adjusted_speed : 999.99;
    }
  else
    /* unmeasurable elapsed time - use big numbers */
    speed = adjusted_speed = (double)999.99;
  
  /* display the speed - no -ve numbers */
  sprintf( message, SPEED_RAW, speed );
  move( SPEED_LINE, COLS - strlen( message ) - 1 );
  ADDSTR_REV( message );
  sprintf( message, SPEED_ADJ,
	   adjusted_speed >= 0.01 ? adjusted_speed : 0.0 );
  move( SPEED_LINE + 1, COLS - strlen( message ) - 1 );
  ADDSTR_REV( message );
  sprintf( message, SPEED_PCT_ERROR,
	   (double)100.0*(double)errcount / (double)total_chars );
  move( SPEED_LINE + 2, COLS - strlen( message ) - 1 );
  ADDSTR_REV( message );
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
    fatal_error( _("invalid key binding (%d:%s)"), line );
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
  int	linenum;			/* line counter */

  /* start at the top of the screen, and clear it */
  linenum = T_TOP_LINE;
  move( linenum, 0 ); clrtobot();

  /* output this line, and each continuation line read */
  do 
    {
      if ( linenum >= LINES - 1 )
	fatal_error( _("data exceeds screen length"), line );
      move( linenum, 0 );
      ADDSTR( SCR_DATA( line ));
      get_script_line( script, line );
      linenum++;
    } 
  while ( SCR_COMMAND( line ) == C_CONT && ! feof( script ));

  /* wait for a return, unless the next command is a query,
     when we can skip it to save the user keystrokes */
  if ( SCR_COMMAND( line ) != C_QUERY )
    wait_user( WAIT_MESSAGE, MODE_TUTORIAL );
  global_prior_command = C_TUTORIAL;
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
  buffer up the complete data from a command; used for D and P
*/
static char *buffer_command( FILE *script, char *line ) {
  int	total_chars = 0;		/* character counter */
  char	*data = NULL;			/* data string */

  /* get the complete exercise into a single string */
  do 
    {
      
      /* allocate space for the extra data */
      if ( total_chars == 0 )
	data = (char*)malloc( strlen(SCR_DATA( line )) +
			      strlen(STRING_NL) + 1 );
      else
	data = (char*)realloc( data, strlen( data ) +
			       strlen(SCR_DATA( line )) +
			       strlen(STRING_NL) + 1 );
      if ( data == NULL )
	fatal_error( _("internal error: malloc"), line );
      
      /* store the data in the allocated area */
      if ( total_chars == 0 )
	strcpy( data, "" );
      strcat( data, SCR_DATA( line ) );
      strcat( data, STRING_NL );
      total_chars = strlen( data );
      
      /* and get the next script line */
      get_script_line( script, line );
    } 
  while ( SCR_COMMAND( line ) == C_CONT && ! feof( script ));

  /* return our (malloced) data */
  return( data );
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
  char	c=0,*p;			 /* char typed in and ptr */
  long	start_time=0, end_time;	 /* timing variables */
  char	message[MAX_WIN_LINE];	 /* message buffer */
  char	drill_type;		 /* note of the drill type */
  int	chars_typed;		 /* count of chars typed */
  bool  seek_done = FALSE;       /* was there a seek_label before exit? */

  /* note the drill type to see if we need to make the user repeat */
  drill_type = SCR_COMMAND( line );

  /* get the complete exercise into a single string */
  data = buffer_command( script, line );

  /* count the lines in this exercise, and check the result
     against the screen length */
  for ( p = data, lines_count = 0; *p != ASCII_NULL; p++ )
    if ( *p == ASCII_NL) lines_count++;
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
      for ( p = data; *p != ASCII_NULL; p++ ) 
	{
	  if ( *p != ASCII_NL )
	    ADDCH( *p );
	  else 
	    {
	      /* newline - move down the screen */
	      linenum++; linenum++;	/* alternate lines */
	      move( linenum, 0 );
	    }
	}
      move( MESSAGE_LINE, COLS - strlen( MODE_DRILL ) - 2 );
      ADDSTR_REV( MODE_DRILL );
    
      /* run the drill */
      linenum = DP_TOP_LINE + 1;
      move( linenum, 0 );
      for ( p = data; *p == ASCII_SPACE && *p != ASCII_NULL; p++ )
	ADDCH( *p );
      for ( chars_typed = 0, errors = 0; *p != ASCII_NULL; p++ ) 
	{
	  rc = getch_fl( *p == ASCII_TAB ?
			 ASCII_TAB : ASCII_SPACE );
	  c = (char)rc;
	  /* this is necessary for DOS: when using raw(), pdcurses 2.4's
	     getch() returns 0x0D on DOS/Windows  */
	  if ( c == 0x0D )
	    rc = c = 0x0A;

	  while ( rc == KEY_BACKSPACE
		  || c == ASCII_BS || c == ASCII_DEL ) 
	    {
	      rc = getch_fl( *p == ASCII_TAB ?
			     ASCII_TAB : ASCII_SPACE );
	      c = (char)rc;
	    }
	
	  /* start timer on first char entered */
	  if ( chars_typed == 0 )
	    start_time = (long)time( NULL );
	  chars_typed++;
	
	  /* ignore delete or backspace in drills */
	  if ( rc == KEY_BACKSPACE ||
	       c == ASCII_BS || c == ASCII_DEL ) 
	    {
	      p--;		/* defeat p++ coming up */
	      continue;
	    }

	  /* ESC is "give up"; ESC at beginning of exercise is "skip lesson"
	     (this is handled outside the for loop) */
	  if ( c == ASCII_ESC )
	    break;
	
	  /* check that the character was correct */
	  if ( c == *p
	       || ( cl_wp_emu && c == ASCII_SPACE
		    && *p == ASCII_NL ))
	    ADDCH( c );
	  else 
	    {
	      ADDCH_REV( *p == ASCII_NL ? DRILL_NL_ERR :
			 (*p == ASCII_TAB ?
			  ASCII_TAB : DRILL_CH_ERR ));
	      if ( ! cl_silent ) 
		{
		  putchar( ASCII_BELL ); fflush( stdout );
		}
	      errors++;
	    }
	
	  /* move screen location if newline */
	  if ( *p == ASCII_NL ) 
	    {
	      linenum++; linenum++;
	      move( linenum, 0 );
	    }

	  /* perform any other word processor like adjustments */
	  if ( cl_wp_emu ) 
	    {
	      if ( c == ASCII_SPACE ) 
		{
		  while ( *(p+1) == ASCII_SPACE
			  && *(p+1) != ASCII_NULL ) 
		    {
		      p++; ADDCH( *p );
		    }
		}
	      else if ( c == ASCII_NL ) 
		{
		  while ( ( *(p+1) == ASCII_SPACE
			    || *(p+1) == ASCII_NL )
			  && *(p+1) != ASCII_NULL ) 
		    {
		      p++; ADDCH( *p );
		      if ( *p == ASCII_NL ) {
			linenum++; linenum++;
			move( linenum, 0 );
		      }
		    }
		}
	      else if ( isalpha(*p) && *(p+1) == ASCII_DASH
			&& *(p+2) == ASCII_NL )	
		{
		  p++; ADDCH( *p );
		  p++; ADDCH( *p );
		  linenum++; linenum++;
		  move( linenum, 0 );
		}
	    }
	}
      
      /* ESC not at the beginning of the lesson: "give up" */
      if ( c == ASCII_ESC && chars_typed != 1)
	continue; /* repeat */

      /* skip timings and don't check error-pct if exit was through ESC */
      if ( c != ASCII_ESC )
	{
	  /* display timings */
	  end_time = (long)time( NULL );
	  if ( ! cl_notimer ) 
	    {
	      display_speed( chars_typed, end_time - start_time,
			     errors );
	    }

	  /* check whether the error-percentage is too high (unless in d:) */
	  if (drill_type != C_DRILL_PRACTICE_ONLY &&
	      (float)errors/(float)chars_typed > global_error_max/100.0) 
	    {
	      sprintf( message, ERROR_TOO_HIGH_MSG, global_error_max );
	      wait_user( message, MODE_DRILL );

	      /* check for F-command */
	      if (global_on_failure_label != NULL)
		{
		  /* move to the label position in the file */
		  if (fseek(script,global_on_failure_label->offset,SEEK_SET )==-1)
		    fatal_error( _("internal error: fseek"), NULL );
		  global_line_counter = global_on_failure_label->line_count;
		  /* tell the user about the misery :) */
		  sprintf(message,SKIPBACK_VIA_F_MSG,
			  global_on_failure_label->label);
		  /* reset value unless persistent */
		  if (!global_on_failure_label_persistent)
		    global_on_failure_label = NULL;
		  wait_user( message, MODE_DRILL );
		  seek_done = TRUE;
		  break;
		}

	      continue;
	    }
	}

      /* ask the user whether he/she wants to repeat or exit */
      if ( c == ASCII_ESC && cl_no_skip ) /* honor --no-skip */
	c = do_query_repeat (script, FALSE);
      else
	c = do_query_repeat (script, TRUE);
      if (c == 'E') {
	seek_done = TRUE;
	break;
      }
      if (c == 'N')
	break;

    }
  
  /* free the malloced memory */
  free( data );

  /* reset global_error_max */
  if (!global_error_max_persistent)
    global_error_max = cl_default_error_max;

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
  char	c=0, *p;		 /* char typed and line ptr */
  long	start_time=0, end_time;	 /* timing variables */
  char	message[MAX_WIN_LINE];	 /* message buffer */
  char	drill_type;		 /* note of the drill type */
  int	chars_typed;		 /* count of chars typed */
  bool  seek_done = FALSE;       /* was there a seek_label before exit? */

  /* note the drill type to see if we need to make the user repeat */
  drill_type = SCR_COMMAND( line );

  /* get the complete exercise into a single string */
  data = buffer_command( script, line );

  /* count the lines in this exercise, and check the result
     against the screen length */
  for ( p = data, lines_count = 0; *p != ASCII_NULL; p++ )
    if ( *p == ASCII_NL) lines_count++;
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
      for ( p = data; *p != ASCII_NULL; p++ ) 
	{
	  if ( *p != ASCII_NL )
	    ADDCH( *p );
	  else
	    {
	      /* newline - move down the screen */
	      linenum++;
	      move( linenum, 0 );
	    }
	}
      move( MESSAGE_LINE, COLS - strlen( MODE_SPEEDTEST ) - 2 );
      ADDSTR_REV( MODE_SPEEDTEST );
  
      /* run the data */
      linenum = DP_TOP_LINE;
      move( linenum, 0 );
      for ( p = data; *p == ASCII_SPACE && *p != ASCII_NULL; p++ )
	ADDCH( *p );
      for ( chars_typed = 0, errors = 0; *p != ASCII_NULL; p++ ) 
	{
	  rc = getch_fl( (*p != ASCII_NL) ? *p : ASCII_SPACE );
	  c = (char)rc;
      
	  /* this is necessary for DOS: when using raw(), pdcurses 2.4's
	     getch() returns 0x0D on DOS/Windows  */
	  if ( c == 0x0D )
	    rc = c = 0x0A;

	  /* start timer on first char entered */
	  if ( chars_typed == 0 )
	    start_time = (long)time( NULL );
	  chars_typed++;
      
	  /* check for delete keys if not at line start or
	     speed test start */
	  if ( rc == KEY_BACKSPACE || c == ASCII_BS || c == ASCII_DEL ) 
	    {
	      /* just ignore deletes where it's impossible or hard */
	      if ( p > data && *(p-1) != ASCII_NL 
		   && *(p-1) != ASCII_TAB ) {
		/* back up one character */
		ADDCH( ASCII_BS ); p--;
	      }
	      p--;		/* defeat p++ coming up */
	      continue;
	    }

	  /* ESC is "give up"; ESC at beginning of exercise is "skip lesson"
	     (this is handled outside the for loop) */
	  if ( c == ASCII_ESC )
	    break;

	  /* check that the character was correct */
	  if ( c == *p
	       || ( cl_wp_emu && c == ASCII_SPACE
		    && *p == ASCII_NL ))
	    ADDCH( c );
	  else 
	    {
	      ADDCH_REV( *p == ASCII_NL ?
			 DRILL_NL_ERR : (unsigned char)*p );
	      if ( ! cl_silent ) {
		putchar( ASCII_BELL ); fflush( stdout );
	      }
	      errors++;
	    }
      
	  /* move screen location if newline */
	  if ( *p == ASCII_NL ) 
	    {
	      linenum++;
	      move( linenum, 0 );
	    }
      
	  /* perform any other word processor like adjustments */
	  if ( cl_wp_emu ) 
	    {
	      if ( c == ASCII_SPACE ) 
		{
		  while ( *(p+1) == ASCII_SPACE
			  && *(p+1) != ASCII_NULL ) 
		    {
		      p++; ADDCH( *p );
		    }
		}
	      else if ( c == ASCII_NL ) 
		{
		  while ( ( *(p+1) == ASCII_SPACE
			    || *(p+1) == ASCII_NL )
			  && *(p+1) != ASCII_NULL ) 
		    {
		      p++; ADDCH( *p );
		      if ( *p == ASCII_NL ) 
			{
			  linenum++;
			  move( linenum, 0 );
			}
		    }
		}
	      else if ( isalpha(*p) && *(p+1) == ASCII_DASH
			&& *(p+2) == ASCII_NL )	
		{
		  p++; ADDCH( *p );
		  p++; ADDCH( *p );
		  linenum++;
		  move( linenum, 0 );
		}
	    }
	}
  
    
      /* ESC not at the beginning of the lesson: "give up" */
      if ( c == ASCII_ESC && chars_typed != 1)
	continue; /* repeat */

      /* skip timings and don't check error-pct if exit was through ESC */
      if ( c != ASCII_ESC )
	{
	  /* display timings */
	  end_time = (long)time( NULL );
	  display_speed( chars_typed, end_time - start_time,
			 errors );
      
	  /* check whether the error-percentage is too high (unless in s:) */
	  if (drill_type != C_SPEEDTEST_PRACTICE_ONLY &&
	      (float)errors/(float)chars_typed > global_error_max/100.0) 
	    {
	      sprintf( message, ERROR_TOO_HIGH_MSG, global_error_max );
	      wait_user( message, MODE_SPEEDTEST );

	      /* check for F-command */
	      if (global_on_failure_label != NULL)
		{
		  /* move to the label position in the file */
		  if (fseek(script,global_on_failure_label->offset,SEEK_SET )==-1)
		    fatal_error( _("internal error: fseek"), NULL );
		  global_line_counter = global_on_failure_label->line_count;
		  /* tell the user about the misery :) */
		  sprintf(message,SKIPBACK_VIA_F_MSG,
			  global_on_failure_label->label);
		  /* reset value unless persistent */
		  if (!global_on_failure_label_persistent)
		    global_on_failure_label = NULL;
		  wait_user( message, MODE_SPEEDTEST );
		  seek_done = TRUE;
		  break;
		}

	      continue;
	    }
	}

      /* ask the user whether he/she wants to repeat or exit */
      if ( c == ASCII_ESC && cl_no_skip ) /* honor --no-skip */
	c = do_query_repeat (script, FALSE);
      else
	c = do_query_repeat (script, TRUE);
      if (c == 'E') {
	seek_done = TRUE;
	break;
      }
      if (c == 'N')
	break;

    }

  /* free the malloced memory */
  free( data );


  /* reset global_error_max */
  if (!global_error_max_persistent)
    global_error_max = cl_default_error_max;

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
static void 
do_clear( FILE *script, char *line ) {
  int	colnum = 0;			/* loop counter */

  /* clear the complete screen */
  move( B_TOP_LINE , 0 ); clrtobot();

  /* create an inverse video banner at the top of the screen */
  for ( colnum = 0; colnum < COLS; colnum++ )
    ADDCH_REV( ASCII_SPACE );

  /* add any banner text */
  move( B_TOP_LINE , 0 );
  ADDSTR_REV( SCR_DATA( line ));

  /* add the version information at top right */
  move( B_TOP_LINE, COLS - (strlen(PACKAGE)+strlen( VERSION )+2) );
  ADDSTR_REV( PACKAGE );
  ADDSTR_REV( " " );
  ADDSTR_REV( VERSION );

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
  exit from the program (implied on eof)
*/
static void 
do_exit( FILE *script ) 
{
  
  /* close up all files, reset the screen stuff, and exit */
  fclose( script );
  if ( cl_colour && has_colors() )
    wbkgdset( stdscr, 0 );
  clear(); refresh(); endwin();
  printf( "\n" );
  exit( 0 );
}


/*
  Ask the user whether he/she wants to repeat, continue or exit
  (this is used at the end of an exercise (drill/speedtest))
  The second argument is FALSE if skipping a lesson isn't allowed (--no-skip).
*/
static char
do_query_repeat ( FILE *script, bool allow_next )
{
  char resp;

  /* display the prompt */
  move( MESSAGE_LINE, 0 ); clrtoeol();
  move( MESSAGE_LINE, COLS - strlen( MODE_QUERY ) - 2 );
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

      if (toupper ((char)resp) == 'R' ||
	  toupper ((char)resp) == RNE [0]) {
	resp = 'R';
	break;
      }
      if (allow_next && (toupper ((char)resp) == 'N' ||
			 toupper ((char)resp) == RNE [2])) {
	resp = 'N';
	break;
      }
      if (toupper ((char)resp) == 'E' || toupper ((char)resp) == RNE [4]) {
	if (fkey_bindings [11] != NULL &&
	    do_query_simple (CONFIRM_EXIT_LESSON_MSG))
	  {
	    seek_label (script, fkey_bindings [11], NULL);
	    resp = 'E';
	    break;
	  }
	/* redisplay the prompt */
	move( MESSAGE_LINE, 0 ); clrtoeol();
	move( MESSAGE_LINE, COLS - strlen( MODE_QUERY ) - 2 );
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
  char resp;

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
      
      if (toupper (resp) == 'Y' || toupper (resp) == YN[0])
	resp = 0;
      if (toupper (resp) == 'N' || toupper (resp) == YN[2])
	resp = -1;
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
      if ( toupper( (char)resp ) == QUERY_Y ||
	   toupper( (char)resp ) == YN[0] ) 
	{
	  ret_code = TRUE;
	  global_resp_flag = TRUE;
	  break;
	}
      if ( toupper( (char)resp ) == QUERY_N ||
	   toupper( (char)resp ) == YN[2] ) 
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
    global_error_max = cl_default_error_max;
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
    if (cl_error_max_specified) {
      if (temp_value < cl_default_error_max)
	global_error_max = temp_value;
      else
	global_error_max = cl_default_error_max;
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
	  
	case C_LABEL:	get_script_line( script, line ); break;
	case C_ERROR_MAX_SET: do_error_max_set( script, line ); break;
	case C_ON_FAILURE_SET: do_on_failure_label_set( script, line ); break;
	default:
	  fatal_error( _("unknown command"), line );
	  break;
	}
    }
}

/**
   indent to column n
   @param n is the amount of times to write ' ' (negative or zero means none)
**/
static void
indent_to( int n )
{
  int i;
  for (i=0; i < n; ++i)
    fputc(' ', stdout);
}

/**
   Prints one usage option. 
   The arguments op, lop and help should not have special characters '\n' '\t'
   @param op is the short option
   @param lop is the long option
   @param help is the explanation of the option
   @param col_op is the column where the short option will be displayed
   @param col_lop is the column where the long option will be displayed
   @param col_op is the column where the explanation will be displayed
   @param last_col is the last column that can be used
**/
static void
print_usage_item( char *op, char *lop, char *help, 
		  int col_op, int col_lop, int col_help, int last_col ) 
{
  int col=0;
  char help_string[MAX_SCR_LINE];
  char *token;
  const char delimiters[] = " ";
  
  assert (op!=NULL && lop!=NULL && help!=NULL);
  assert (0<=col_op && col_op<col_lop 
	  && col_lop<col_help && col_help<last_col);
  
  indent_to(col_op);
  printf ("%s", op);
  col += col_op + strlen (op);
  
  indent_to(col_lop - col);
  printf ("%s", lop);
  col += MAX(0, col_lop - col) + strlen (lop);

  indent_to(col_help - col);
  col += MAX(0, col_help - col);
 
  strcpy ( help_string, help );
  token = strtok ( help_string, delimiters );
  while (token != NULL)
    {
      if (col + strlen (token) >= last_col) {
	putc ( '\n', stdout );
	indent_to ( col_help );
	fputs ( token, stdout );
	putc ( ' ', stdout );
	col = col_help + strlen(token) + 1;
      } else {
	fputs ( token, stdout );
	putc ( ' ', stdout );
	col += strlen(token) + 1;
      }
      token = strtok ( NULL, delimiters );
    }
  putc('\n', stdout);
}


/**
   Prints usage instructions
**/
static void
print_help()
{
  char *op[]= 
    { "-e %",
      "-n",
      "-t",
      "-f P",
      "-c F,B",
      "-s",
      "-q",
      "-l L",
      "-w",
      "-k",
      "-h",
      "-v" };
  char *lop[]= 
    { "--max-error=%",
      "--notimer",
      "--term-cursor",
      "--curs-flash=P",
      "--colours=F,B",
      "--silent",
      "--quiet",
      "--start-label=L",
      "--word-processor",
      "--no-skip",
      "--help",
      "--version" };
  char *help[] = 
    { 
      _("default maximum error percentage (default 3.0); valid values are between 0.0 and 100.0"),
      _("turn off WPM timer in drills"),
      _("use the terminal's hardware cursor"),
      _("cursor flash period P*.1 sec (default 10); valid  values are between 0 and 512; this is ignored if -t is specified"),
      _("set initial display colours where available"),
      _("don't beep on errors"),
      _("same as -s, --silent"),
      _("start the lesson at label 'L'"),
      _("try to mimic word processors"),
      _("forbid the user to skip exercises"),
      _("print this message"),
      _("output version information and exit") };
  
  int loop;
  
  printf(_("`gtypist' is a typing tutor with several lessons for different keyboards and languages.  New lessons can be written by the user easily.\n\n"));
  printf("%s: %s [ %s... ] [ %s ]\n\n",
	 _("Usage"),argv0,_("Options"),_("script-file"));
  printf("%s:\n",_("Options"));
  /* print out each line of the help text array */
  for ( loop = 0; loop < sizeof(help)/sizeof(char *); loop++ ) 
    {
      print_usage_item( op[loop], lop[loop], help[loop], 1, 8, 25, 75 );
    }

  printf(_("\nIf not supplied, script-file defaults to '%s/%s'.\n")
	 ,PACKAGE_DATA_DIR,DEFAULT_SCRIPT);
  printf(_("The path $GTYPIST_PATH is searched for script files.\n\n"));

  printf("%s:\n",_("Examples"));
  printf("  %s:\n    %s\n\n",
	 _("To run the default lesson in english `gtypist.typ'"),argv0);
  printf("  %s:\n    %s esp.typ\n\n",
	 _("To run the lesson in spanish"),argv0);
  printf("  %s:\n    GTYPIST_PATH=\"/home/foo\" %s bar.typ\n\n",
	 _("To instruct gtypist to look for lesson `bar.typ' in a non standard directory"),argv0);
  printf("  %s:\n    %s -t -q -l TEST1 /temp/test.typ\n\n",
	 _("To run the lesson in the file `test.typ' of directory `temp', starting at label `TEST1', using the terminal's cursor, and run silently"),argv0);
  printf("%s\n",_("Report bugs to bug-gtypist@gnu.org")); 
}


/*
  parse command line options for initial values
*/
static void 
parse_cmdline( int argc, char **argv ) {
  int	c;				/* option character */
  int	option_index;			/* option index */
  static struct option long_options[] = {	/* options table */
    { "max-error",      required_argument, 0, 'e' },
    { "notimer",	no_argument, 0, 'n' },
    { "term-cursor",	no_argument, 0, 't' },
    { "curs-flash",	required_argument, 0, 'f' },
    { "colours",	required_argument, 0, 'c' },
    { "colors",		required_argument, 0, 'c' },
    { "silent",		no_argument, 0, 's' },
    { "quiet",		no_argument, 0, 'q' },
    { "start-label",	required_argument, 0, 'l' },
    { "word-processor",	no_argument, 0, 'w' },
    { "no-skip",        no_argument, 0, 'k' },
    { "help",		no_argument, 0, 'h' },
    { "version",	no_argument, 0, 'v' },
    { 0, 0, 0, 0 }};

  /* process every option */
  while ( (c=getopt_long( argc, argv, "e:ntf:c:sql:wkhv",
			  long_options, &option_index )) != -1 ) 
    {
      switch (c)
	{
	case 'e':
	  cl_error_max_specified = TRUE;
	  if ( sscanf( optarg, "%f", &cl_default_error_max ) != 1 
	       || cl_default_error_max < 0.0
	       || cl_default_error_max > 100.0 ) 
	    {
	      fprintf( stderr, _("%s: invalid error-max value\n"),
		       argv0 );
	      exit( 1 );
	    }
	  break;
	case 'n':
	  cl_notimer = TRUE;
	  break;
	case 't':
	  cl_term_cursor = TRUE;
	  break;
	case 'f':
	  if ( sscanf( optarg, "%d", &cl_curs_flash ) != 1
	       || cl_curs_flash < 0
	       || cl_curs_flash > 512 ) 
	    {
	      fprintf( stderr, _("%s: invalid curs-flash value\n"),
		       argv0 );
	      exit( 1 );
	    }
	  break;
	case 'c':
	  if ( sscanf( optarg, "%d,%d",
		       &cl_fgcolour, &cl_bgcolour ) != 2 ||
	       cl_fgcolour < 0 || cl_fgcolour >= NUM_COLOURS ||
	       cl_bgcolour < 0 || cl_bgcolour >= NUM_COLOURS ) 
	    {
	      fprintf( stderr, _("%s: invalid colours value\n"),argv0 );
	      exit( 1 );
	    }
	  cl_colour = TRUE;
	  break;
	case 's':
	case 'q':
	  cl_silent = TRUE;
	  break;
	case 'l':
	  cl_start_label = (char*)malloc( strlen( optarg ) + 1 );
	  if ( cl_start_label == NULL ) 
	    {
	      fprintf( stderr, _("%s: internal error: malloc\n"),argv0 );
	      exit( 1 );
	    }
	  strcpy( cl_start_label, optarg );
	  break;
	case 'w':
	  cl_wp_emu = TRUE;
	  break;
	case 'k':
	  cl_no_skip = TRUE;
	  break;
	case 'h':
	  print_help();
	  exit( 0 );
	  break;
	case 'v':
	  printf( "%s %s\n\n", PACKAGE,VERSION );
	  printf( "%s\n\n", COPYRIGHT );
	  printf( "%s\n", _("Written by Simon Baldwin"));
	  exit( 0 );
	  break;
	case '?':
	default:
	  fprintf( stderr,
		   _("Try '%s --help' for more information.\n"), argv0 );
	  exit( 1 );
	}
    }
  if ( argc - optind > 1 ) 
    {
      fprintf( stderr,
	       _("Try '%s --help' for more information.\n"), argv0 );
      exit( 1 );
    }
}


/*
  signal handler to stop curses stuff on intr
*/
static void 
catcher( int signal ) {
  
  /* unravel colours and curses, clean up and exit */
  if ( cl_colour && has_colors() )
    wbkgdset( stdscr, 0 );
  clear(); refresh(); endwin();
  printf("\n");
  exit( 1 );
}

/*
  main routine
*/
int main( int argc, char **argv ) 
{
  WINDOW	*scr;				/* curses window */
  FILE	*script;			/* script file handle */
  int	colnum = 0;			/* column counter */
  char	*p, filepath[FILENAME_MAX];	/* file paths */
  char	script_file[FILENAME_MAX];	/* more file paths */
  
  /* Internationalization */
  
#if defined(ENABLE_NLS) && defined(LC_ALL)
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  COPYRIGHT=_("Copyright (C) 1998,2001  Simon Baldwin.\n\
This program comes with ABSOLUTELY NO WARRANTY; for details\n\
please see the file 'COPYING' supplied with the source code.\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions; again, see 'COPYING' for details.\n\
This program is released under the GNU General Public License.");
  /* this string is displayed in the mode-line when in a tutorial */
  MODE_TUTORIAL=_(" Tutorial ");
  /* this string is displayed in the mode-line when in a query */
  MODE_QUERY=_("  Query   ");
  /* this string is displayed in the mode-line when running a drill */
  MODE_DRILL=_("  Drill   ");
  /* this string is displayed in the mode-line when running a speedtest */
  MODE_SPEEDTEST=_("Speed test");
  WAIT_MESSAGE=_(" Press Return to continue... ");
  /* this message is displayed when the user has failed in a [DS]: drill */
  ERROR_TOO_HIGH_MSG=
    _("Your error-rate is too high. You have to achieve %.1f%%.");
  /* this message is displayed when the user has failed in a [DS]: drill,
     and an F:<LABEL> ("on failure label") is in effect */
  SKIPBACK_VIA_F_MSG=
    _("You failed this test, so you need to go back to %s.");
  /* this is used for repeat-queries. you can translate the keys as well
     (if you translate msgid "R/N/E" accordingly) */
  REPEAT_NEXT_EXIT_MSG= 
	_("Press R to repeat, N for next exercise or E to exit");
  /* this is used for repeat-queries with --no-skip. you can translate
     the keys as well (if you translate msgid "R/N/E" accordingly) */
  REPEAT_EXIT_MSG= 
	_("Press R to repeat or E to exit");
  /* This is used make the user confirm (E)xit in REPEAT_NEXT_EXIT_MSG */
  CONFIRM_EXIT_LESSON_MSG=
    _("Are you sure you want to exit this lesson? [Y/N]");
  /* This message is displayed if the user tries to skip a lesson
     (ESC ESC) and --no-skip is specified */
  NO_SKIP_MSG= 
    _("Sorry, gtypist is configured to forbid skipping exercises.");
  /* this must be adjusted to the right with one space at the end.
     Leading whitespace is important because it is displayed in reverse
     video because it must be aligned with the next two messages 
     (it's best to run gtypist to see this in practice)   */
  SPEED_RAW=_(" Raw speed      = %6.2f wpm ");
  /* this must be adjusted to the right with one space at the end.
     Leading whitespace is important because it is displayed in reverse
     video and because it must be aligned with the previous and next messages
     (it's best to run gtypist to see this in practice)   */
  SPEED_ADJ=  _(" Adjusted speed = %6.2f wpm ");
  /* this must be adjusted to the right with one space at the end.
     Leading whitespace is important because it is displayed in reverse
     video and because it must be aligned with the next last two messages
     (it's best to run gtypist to see this in practice)   */
  SPEED_PCT_ERROR=_("            with %.1f%% errors ");
  /* this is used to translate the keys for Y/N-queries. Must be two
     uppercase letters separated by '/'. Y/N will still be accepted as
     well. Note that the messages (prompts) themselves cannot be
     translated because they are read from the script-file. */
  YN = _("Y/N");
  if (strlen(YN) != 3 || YN[1] != '/' || !isupper(YN[0]) || !isupper(YN[2]))
    {
      fprintf( stderr,
	       "%s: i18n problem: invalid value for msgid \"Y/N\": %s\n",
	       argv0, YN );
      exit( 1 );
    }
  /* this is used to translate the keys for Repeat/Next/Exit
     queries. Must be three uppercase letters separated by slashes. */
  RNE = _("R/N/E");
  if (strlen(RNE) != 5 ||
      !isupper(RNE[0]) || RNE[1] != '/' ||
      !isupper(RNE[2]) || RNE[3] != '/' ||
      !isupper(RNE[4]))
    {
      fprintf( stderr,
	       "%s: i18n problem: invalid value for msgid \"R/N/E\": %s\n",
	       argv0, RNE );
      exit( 1 );
    }
  
  /* get our name for error messages */
  argv0 = argv[0] + strlen( argv[0] );
  while ( argv0 > argv[0] && *argv0 != '/' )
    argv0--;
  if ( *argv0 == '/' ) argv0++;
  
  /* check usage and open input file */
  parse_cmdline( argc, argv );
  global_error_max = cl_default_error_max;
  if ( argc - optind == 1 )
    strcpy( script_file, argv[optind] );
  else
    sprintf( script_file,"%s/%s",PACKAGE_DATA_DIR,DEFAULT_SCRIPT);
  
  script = fopen( script_file, "r" );
  if (script==NULL && getenv( "GTYPIST_PATH" ) != NULL ) 
    {
      p = strtok( getenv( "GTYPIST_PATH" ), ":" );
      for ( ; p != NULL;
	    p = strtok( NULL, ":" )) 
	{
	  strcpy( filepath, p );
	  strcat( filepath, "/" );
	  strcat( filepath, script_file );
	  if ( (script = fopen( filepath, "r" )) != NULL )
	    break;
	}
    }
  if (script==NULL)
    {
      strcpy( filepath, PACKAGE_DATA_DIR );
      strcat( filepath, "/" );
      strcat( filepath, script_file );
      script = fopen( filepath, "r" );
    }
  
  if ( script == NULL ) 
    {
      fprintf( stderr, "%s: %s %s\n",
	       argv0, _("can't find or open file"),script_file );
      exit( 1 );
    }
  
  /* prepare for curses stuff, and set up a signal handler
     to undo curses if we get interrupted */
  scr = initscr();
  signal( SIGHUP, catcher );  signal( SIGINT, catcher );
  signal( SIGQUIT, catcher ); 
#ifndef DJGPP
  signal( SIGCHLD, catcher );
#endif
  signal( SIGPIPE, catcher ); signal( SIGTERM, catcher );
  clear(); refresh(); typeahead( -1 );
  keypad( scr, TRUE ); noecho(); curs_set( 0 ); raw();
  
  /* set up colour pairs if possible */
  if ( cl_colour && has_colors() ) 
    {
      start_color();
      init_pair( C_NORMAL,
		 colour_array[ cl_fgcolour ],
		 colour_array[ cl_bgcolour ] );
      wbkgdset( stdscr, COLOR_PAIR( C_NORMAL ) );
    }
  
  /* put up the top line banner */
  clear();
  move( B_TOP_LINE , 0 );;
  for ( colnum = 0; colnum < COLS; colnum++ )
    ADDCH_REV( ASCII_SPACE );
  move( B_TOP_LINE, COLS - strlen(PACKAGE) - strlen(VERSION)+2 );
  ADDSTR_REV( PACKAGE );
  ADDSTR_REV( " " );
  ADDSTR_REV( VERSION );
  
  /* index all the labels in the file */
  index_labels( script );
  
  /* run the input file */
  parse_file( script, cl_start_label );
  do_exit( script );

  /* for lint... */
  return( 0 );
}
