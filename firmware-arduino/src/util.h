// Common utilities

#ifndef util_h_included
#define util_h_included

#include "main.h"
#include "log.h"

// Return the nth blank or quote delimited word from the line, or "" if there is no such word.
// If a word starts with '"' then it is assumed to be quoted, and we scan until the closing '"'
// and return the content between the quotes.  Ditto for single quote.  If the matching quote
// is missing then we fall back to separating by blanks.  If flag != nullptr then *flag
// is set to true if a word was found or false if not, this can be used to distinguish
// an empty quoted word from no word.

String get_word(const String& line, int n, bool* flag = nullptr);

// Reads nonempty lines terminated by CR, LF, or CRLF, but note there's no line editing
// unless the front end (terminal, whatever) implements it.
//
// Control characters are accepted but not usually what you want.

String blocking_read_nonempty_line(Stream* io);

// Format stuff into a String.

String fmt(const char* format, ...) __attribute__ ((format (printf, 1, 2)));

// Starting at *p, look for `something=something_else` followed by & or end of string (NUL).
// The `something_else` is url-encoded.  Return true if we found a matching pair, and assign
// *key and *value (url-decoded) and update *p.  Otherwise return false and leave *p unchanged.
// Note that `something_else` can be an empty string.

bool get_posted_field(const char** p, String* key, String* value);

// Format the timestamp in a standard way.

String format_timestamp(time_t t);

// Emit message on possible channels and do not return.

void panic(const char* msg) NO_RETURN;

// List of T.

template<typename T>
class List {
  struct Node {
    Node(T&& value) : value(std::move(value)) {}
    T value;
    Node* next = nullptr;
  };

  // Invariant: (first == nullptr) == (last == nullptr)
  // Invariant: first == nullptr || first->next->...->next == last
  // Invariant: last == nullptr || last->next = nullptr;
  Node* first = nullptr;
  Node* last = nullptr;

public:
  bool is_empty() const {
    return first == nullptr;
  }

  void clear() {
    while (!is_empty()) {
        pop_front();
    }
  }

  void add_back(T&& value) {
    Node* node = new Node(std::move(value));
    if (last == nullptr) {
        first = last = node;
    } else {
        last->next = node;
        last = node;
    }
  }

  T& peek_front() const {
    if (first == nullptr) {
      panic("Empty list");
    }
    return first->value;
  }

  T pop_front() {
    if (first == nullptr) {
      panic("Empty list");
    }
    Node* node = first;
    first = node->next;
    if (first == nullptr) {
      last = nullptr;
    }
    T value = std::move(node->value);
    delete node;
    return value;
  }
};

#endif // !util_h_included
