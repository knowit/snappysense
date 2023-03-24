// Support for the device acting as a simple web server

#include "web_server.h"

#ifdef SNAPPY_WEB_SERVER

#include "command.h"
#include "config.h"
#include "device.h"
#include "log.h"
#include "network_wifi.h"
#include "util.h"

#include <WiFi.h>
#include <WiFiClient.h>

// HTTP request parsing state.

enum class RequestParseState {
  TEXT,
  CR,
  CRLF,
  CRLFCR,
  CRLFCRLF,
};

// One request handler object for each connection created to the server,
// this packages a WiFiClient with input parsing state and some bookkeeping.

struct WebRequestHandler {
  WebRequestHandler(WiFiClient&& client) : client(std::move(client)) {}
  ~WebRequestHandler() {}

  // The WiFi client for this web client
  WiFiClient client;

  // List of clients attached to the unique server
  WebRequestHandler* next = nullptr;

  // Input parsing state for this client
  RequestParseState state = RequestParseState::TEXT;

  // This is the request that has been collected for processing.
  String request;

  // process_request() and failed_request() set `dead` to true when the client is done.
  bool dead = false;

  // set to true once the request has been dispatched to the main thread and we
  // should no longer be listening
  bool complete = false;
};

static WebRequestHandler* request_handlers;
static WiFiServer* web_server;
static TimerHandle_t web_timer;

void web_server_init(int port) {
  if (web_server) {
    panic("Multiple web servers");
  }
  web_timer = xTimerCreate("web", pdMS_TO_TICKS(1000), pdTRUE, nullptr,
                           [](TimerHandle_t){ put_main_event(EvCode::WEB_SERVER_POLL); });
  web_server = new WiFiServer(port);
  web_server->begin();
}

void web_server_start() {
  xTimerStart(web_timer, portMAX_DELAY);
}

void web_server_stop() {
  xTimerStop(web_timer, portMAX_DELAY);
}

// Parse the input until it is terminated.  If the input was complete, invoke the processing
// function to handle it.  If the input was incomplete and there isn't any more, invoke the
// failure function to signal this.
//
// The request is a number of CRLF-terminated lines followed by a blank CRLF-terminated line.
// This state machine attempts to implement that precisely.  It may be that a looser
// interpretation would be more resilient.

static void poll(WebRequestHandler* rh) {
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
  log("Web: finished request, %d\n", (int)rh->state);
  rh->complete = true;
  if (rh->state == RequestParseState::CRLFCRLF) {
    put_main_event(EvCode::WEB_REQUEST, new WebRequest(rh->request, rh->client));
  } else {
    put_main_event(EvCode::WEB_REQUEST_FAILED, new WebRequest(rh->request, rh->client));
  }
}

void web_server_poll() {
  if (!web_server) {
    return;
  }

  // Listen for incoming clients
  for (;;) {
    WiFiClient client = web_server->available();
    if (!client) {
      break;
    }
    log("Web server: Incoming request\n");
    WebRequestHandler* rh = new WebRequestHandler(std::move(client));
    rh->next = request_handlers;
    request_handlers = rh;
  }

  // Obtain traffic and respond to requests.  I guess in principle this should
  // be two loops, where the outer loop runs as long as some work has been
  // performed, but it doesn't seem important.
  for ( WebRequestHandler* rh = request_handlers; rh != nullptr; rh = rh->next ) {
    if (!rh->complete && !rh->dead) {
      poll(rh);
    }
  }

  // Garbage collect the clients
  WebRequestHandler* curr = request_handlers;
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
        request_handlers = next;
      } else {
        prev->next = next;
      }
      delete curr;
      curr = next;
    } else {
      prev = curr;
      curr = curr->next;
    }
  }
}

void web_server_request_completed(WebRequest* r) {
  log("Reaping client\n");
  for ( WebRequestHandler* rh = request_handlers; rh != nullptr; rh = rh->next ) {
    if (&rh->client == &r->client) {
      rh->dead = true;
      break;
    }
  }
  delete r;
}

#endif // SNAPPY_WEB_SERVER
