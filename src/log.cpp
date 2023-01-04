// Logging functionality
//
// This is a little more complicated than we'd like it to be, in order to support
// printing characters as ASCII codes when not printable.

#include "log.h"

#ifdef LOGGING
static Stream* log_stream_;

void set_log_stream(Stream* s) {
  log_stream_ = s;
}

void log(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
    va_log(fmt, args);
    va_end(args);
}

void va_log(const char* fmt, va_list args) {
  if (!log_stream_) {
    return;
  }
  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt++) {
        case 'd':
          log_stream_->printf("%d", va_arg(args, int));
          break;
        case 's':
          log_stream_->printf("%s", va_arg(args, char*));
          break;
        case 'c': {
          int c = va_arg(args, int);
          if (c < 32) {
            log_stream_->printf("%02x", c);
          } else {
            log_stream_->printf("%c", c);
          }
          break;
        }
        case '%':
          log_stream_->printf("%%");
          break;
        default:
          return;
      }
    } else {
      log_stream_->printf("%c", *fmt++);
      break;
    }
  }
}
#endif
