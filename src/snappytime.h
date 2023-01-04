// Stuff to obtain the current time from a web server and to obtain timestamps with that adjustment.
//
// The remote server must know how to handle GET to /time; it must respond with a payload that is
// the decimal encoding of the number of seconds elapsed since the Posix epoch (ie, what time()
// would return on a properly configured Posix system).  For a simple server that can do this, see
// `github.com/lars-t-hansen/sandbox/http`.
//
// larhan@knowit.no

#ifndef snappytime_h_included
#define snappytime_h_included

#include "main.h"

#ifdef TIMESTAMP
void configure_time();
time_t get_time();
struct tm snappy_local_time();
#endif

#endif // !snappytime_h_included
