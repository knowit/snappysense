#ifndef util_h_included
#define util_h_included

#include "main.h"

// Return the nth blank-delimited word from the line, or "" if there is no such word.
// If a word starts with '"' then it is assumed to be quoted, and we scan until the
// closing '"' and return the content between the quotes.  Ditto for single quote.
// If the matching quote is missing then we fall back to separating by blanks.

String get_word(const String& line, int n);

// Reads *nonempty* lines terminated by CR, LF, or CRLF, but note there's no line editing
// unless the front end (terminal, whatever) implements it.
//
// Control characters are accepted but not usually what you want.

String blocking_read_nonempty_line(Stream* io);

#endif // !util_h_included
