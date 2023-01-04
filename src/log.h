// Logging functionality

#ifndef log_h_included
#define log_h_included

#include "main.h"
#include <Stream.h>
#include <cstdarg>

#ifdef LOGGING
void set_log_stream(Stream* output);

// Printf-like formatting but restricted for now to %s, %d and %c.  Don't be fancy!
// %c prints the character if printable, otherwise the ascii code.
void log(const char* fmt, ...);
void va_log(const char* fmt, va_list args);
#else
void set_log_stream(Stream* output) {
  /* Nothing */
}
static inline void log(const char* fmt, ...) {
  /* Nothing */
}
static inline void va_log(const char* fmt, va_list) {
  /* Nothing */
}
#endif

#endif // !log_h_included