// Common utilities

#include "util.h"
#include "device.h"
#include <cstdarg>

String get_word(const String& cmd, int n, bool* flag) {
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
        i++;
    } else if (i < lim && cmd[i] == '\'') {
        quoted = '\'';
        i++;
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
      if (flag != nullptr) {
        *flag = true;
      }
      if (quoted) {
        return cmd.substring(start+1, i-1);
      }
      return cmd.substring(start, i);
    }
    n--;
  }
  if (flag != nullptr) {
    *flag = false;
  }
  return String();
}

String blocking_read_nonempty_line(Stream* input) {
  String buf;
  for (;;) {
    int ch = input->read();
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

String fmt(const char* format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  return String(buf);
}

String format_time(const struct tm& time) {
  static const char* const weekdays[] = {
    "sun", "mon", "tue", "wed", "thu", "fri", "sat"
  };
  char buf[256];
  // Timestamp format defined in aws-iot-backend/mqtt-protocol.md.
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d/%s",
           time.tm_year + 1900,     // year number
           time.tm_mon + 1,         // month, 1-12
           time.tm_mday,            // day of the month, 1-31
           time.tm_hour,            // hour, 0-23
           time.tm_min,             // minute, 0-59
           weekdays[time.tm_wday]); // day of the week
  return String(buf);
}

void panic(const char* msg) {
  log("Panic: %s\n", msg);
  enter_end_state(msg, true);
}

static int hexval(char c) {
  if (c <= '9') {
    return c - '0';
  }
  if (c <= 'Z') {
    return c - 'A' + 10;
  }
  return c - 'a' + 10;
}

bool get_posted_field(const char** p, String* key, String* value) {
  const char* q = *p;
  while (*q && *q != '=') {
    q++;
  }
  if (!*q) {
    return false;
  }
  *key = String(*p, q - *p);
  q++;
  const char* start = q;
  while (*q && *q != '&') {
    q++;
  }
  *value = "";
  const char *r = start;
  while (r < q) {
    if (*r == '+') {
      *value += ' ';
      r++;
    } else if (*r == '%') {
      *value += (char)((hexval(*(r+1)) << 4) | hexval(*(r+2)));
      r += 3;
    } else {
      *value += *r++;
    }
  }
  if (*q) {
    q++;
  }
  *p = q;
  return true;
}
