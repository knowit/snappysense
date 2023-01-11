#include "util.h"

String get_word(const String& cmd, int n) {
  unsigned lim = cmd.length();
  unsigned i = 0;
  while (i < lim) {
    while (i < lim && isspace(cmd[i])) {
      i++;
    }
    unsigned start = i;
    int quoted = 0;
    if (i < lim && cmd[i] == '"') {
        quoted = '"';
    } else if (i < lim && cmd[i] == '\'') {
        quoted = '\'';
    }
  again:
    for (;;) {
      if (i == lim) {
        if (quoted) {
          // Missing closing quote, so redo the scan without quoting
          i = start;
          quoted = 0;
          goto again;
        }
        break;
      }
      if (quoted && cmd[i] == quoted) {
        i++;
        break;
      } 
      if (!quoted && isspace(cmd[i])) {
        break;
      }
      i++;
    }
    if (i == start) {
      // End of input
      break;
    }
    if (n == 0) {
      if (quoted) {
        return cmd.substring(start+1, i-1);
      }
      return cmd.substring(start, i);
    }
    n--;
  }
  return String();
}

String blocking_read_nonempty_line(Stream* io) {
  String buf;
  for (;;) {
    int ch = io->read();
    if (ch <= 0 || ch > 127) {
      continue;
    }
    if (ch == '\r' || ch == '\n') {
      if (buf.isEmpty()) {
        continue;
      }
      return buf;
    }
    buf += (char)ch;
  }
}
