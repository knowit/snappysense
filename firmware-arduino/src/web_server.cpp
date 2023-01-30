// Support for the device acting as a simple web server, for configuration and commands

#include "web_server.h"
#include "device.h"

#ifdef WEB_SERVER

#include "command.h"
#include "config.h"
#include "log.h"
#include "network.h"
#include "util.h"

#include <WiFi.h>
#include <WiFiClient.h>

// Parse the input until it is terminated.  If the input was complete, invoke the processing
// function to handle it.  If the input was incomplete and there isn't any more, invoke the
// failure function to signal this.
//
// The request is a number of CRLF-terminated lines followed by a blank CRLF-terminated line.
// This state machine attempts to implement that precisely.  It may be that a looser
// interpretation would be more resilient.

void WebInputTask::poll(WebRequestHandler* rh) {
  // Client can disconnect at any time
  while (rh->client.connected()) {
    // Bytes arrive now and then.
    while (rh->client.available()) {
      char c = rh->client.read();
      //log("Web: received %c\n", c);
      switch (rh->state) {
      case RequestParseState::TEXT:
        if (c == '\r') {
          rh->state = RequestParseState::CR;
        }
        break;
      case RequestParseState::CR:
        if (c == '\n') {
          rh->state = RequestParseState::CRLF;
        } else if (c == '\r') {
          rh->state = RequestParseState::CR;
        } else {
          rh->state = RequestParseState::TEXT;
        }
        break;
      case RequestParseState::CRLF:
        if (c == '\r') {
          rh->state = RequestParseState::CRLFCR;
        } else {
          rh->state = RequestParseState::TEXT;
        }
        break;
      case RequestParseState::CRLFCR:
        if (c == '\n') {
          rh->state = RequestParseState::CRLFCRLF;
          goto request_completed;
        } else if (c == '\r') {
          rh->state = RequestParseState::CR;
        } else {
          rh->state = RequestParseState::TEXT;
        }
        break;
      case RequestParseState::CRLFCRLF:
        // Should not happen
        rh->state = RequestParseState::TEXT;
        goto request_completed;
      }
      rh->request += c;
    }
    // Connected (probably) but input not available and not final state, come back later.
    return;
  }

request_completed:
  if (rh->state == RequestParseState::CRLFCRLF) {
    rh->process_request();
  } else {
    rh->failed_request();
  }
}

// This polls the server for new clients, and for each active client, polls for input.
// It then reaps any dead clients.

void WebInputTask::execute(SnappySenseData*) {
  if (web_server == nullptr) {
    if (!start()) {
      return;
    }
  }

  // Listen for incoming clients
  for (;;) {
    WiFiClient client = web_server->server.available();
    if (!client) {
      break;
    }
    log("Web server: Incoming request\n");
    WebRequestHandler* rh = create_request_handler(std::move(client));
    rh->next = web_server->request_handlers;
    web_server->request_handlers = rh;
  }

  // Obtain traffic and respond to requests.  I guess in principle this should
  // be two loops, where the outer loop runs as long as some work has been
  // performed, but it doesn't seem important.
  for ( WebRequestHandler* rh = web_server->request_handlers; rh != nullptr; rh = rh->next ) {
    if (!rh->dead) {
      poll(rh);
    }
  }

  // Garbage collect the clients
  WebRequestHandler* curr = web_server->request_handlers;
  WebRequestHandler* prev = nullptr;
  while (curr != nullptr) {
    if (curr->dead) {
      // Weird, "flush" actually reads any pending input, it does not write
      // any pending output.  write() should send all the output, it blocks
      // until it's sent.  It looks like print, println etc end up calling write,
      // so we should be OK at this point - output should have been sent.
      // And in addition, flush() may only be meaningful if the client is still
      // connected.  So skip it.
      //curr->client.flush();
      curr->client.stop();
      WebRequestHandler* next = curr->next;
      if (prev == nullptr) {
        web_server->request_handlers = next;
      } else {
        prev->next = next;
      }
      delete curr;
      curr = next;
    }
  }
}

#endif // WEB_SERVER
