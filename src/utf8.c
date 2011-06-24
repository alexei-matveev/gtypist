#include "utf8.h"
#include <ncursesw/ncurses.h>

void wideaddch(wchar_t c)
{
  cchar_t c2;
  attr_get(&c2.attr, &c2.ext_color, NULL);
  c2.chars[0] = c;
  c2.chars[1] = L'\0';
  add_wch(&c2);
}

void wideaddch_rev(wchar_t c)
{
  attron(A_REVERSE);
  wideaddch(c);
  attroff(A_REVERSE);
}

int mbslen(char* str)
{
  return mbstowcs(NULL, str, 0);
}

/*
  Local Variables:
  tab-width: 8
  End:
*/
