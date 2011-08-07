#include "utf8.h"
#include <ncursesw/ncurses.h>
#include <stdlib.h>
#include "gettext.h"
#define _(String) gettext (String)

void wideaddch(wchar_t c)
{
  cchar_t c2;
  int dummy;
  attr_get(&c2.attr, &dummy, NULL);
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

int mbslen(const char* str)
{
  return mbstowcs(NULL, str, 0);
}

wchar_t* widen(const char* text)
{
    int numChars = mbslen(text);
    wchar_t* wideText = malloc((numChars+1) * sizeof(wchar_t));
    int convresult = mbstowcs(wideText, text, numChars+1);
    if (convresult != numChars)
        fatal_error(_("couldn't convert UTF-8 to wide characters"), "?");
    return wideText;
}

/*
  Local Variables:
  tab-width: 8
  End:
*/
