#ifndef SCRIPT_H
#define SCRIPT_H

#include <stdio.h>

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
#define C_MENU                  'M'

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

/* limits on line lengths from the script file and screen */
#define	MAX_SCR_LINE		1024
#define	MIN_SCR_LINE		2
#define	MAX_WIN_LINE		128

extern int global_line_counter;

extern char *__last_label;

/* a global area for label indexing - singly linked lists, hashed */
#define	NLHASH			32		/* num hash lists */
struct label_entry {
  char		*label;			/* label string */
  long		offset;			/* offset into file */
  int		line_count;		/* line number in script */
  struct label_entry	*next;		/* pointer to next element */
};
extern struct label_entry *global_label_list[NLHASH];


extern void get_script_line( FILE *script, char *line );
extern char *buffer_command( FILE *script, char *line );
extern void seek_label( FILE *script, char *label, char *ref_line );
extern int hash_label( char *label );
extern void do_exit( FILE *script );


#endif /* !SCRIPT_H */

