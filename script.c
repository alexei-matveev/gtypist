#include "config.h"
#include "script.h"
#ifdef HAVE_LIBCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif
#include "gettext.h"
#include "error.h"

int global_line_counter = 0;
struct label_entry *global_label_list[NLHASH];

/*
  label hashing function - returns an index for the lists
*/
int 
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
  get the next non-comment, non-blank line from the script file
  and check its basic format
*/
void 
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
      /* Get rid of trailing spaces and newline */
      while( *line && isspace( line[strlen( line )-1] ) )
        line[strlen( line )-1] = ASCII_NULL;

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
  buffer up the complete data from a command; used for D and P
*/
char *buffer_command( FILE *script, char *line ) {
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
  search out a label from the file, and set the file pointer to
  that location
*/
void 
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
