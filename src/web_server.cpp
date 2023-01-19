// Support for the device acting as a simple web server, for configuration and commands

// Configuration / provisioning:
//
// The device acts as an access point, and its SSID and IP address are displayed on the
// device screen when the device is in configuration mode.  The user's device connects
// to this SSID, and loads the top-level page from the IP / port 80 - that is, just typing
// the IP address into the browser address bar is enough, no port or path is needed.
// This action brings up a configuration form, which can be filled in and submitted to store
// the values; the process can be repeated.

// Command processing:
//
// The device is connected to an external access point, and the device has an IP
// address provided by that AP.  It listens on a port, default 8086, for GET
// requests that ...
//
// TODO: The IP must be shown on the device screen, not just on the serial port.
// It can be shown by the slideshow if the slideshow is running, otherwise whenever
// the device wakes up to read the sensors.
//
// Parameters are passed using the usual syntax, so <http://.../get?temperature> to
// return the temperature reading.  The parameter handling is ad-hoc and works only
// for these simple cases.

#include "web_server.h"
#include "device.h"

#ifdef WEB_SERVER

// If WEB_SERVER is on then either WEB_COMMAND_SERVER or WEB_CONFIGURATION must
// be on, but not both.  Code below depends on this.

#if (!defined(WEB_COMMAND_SERVER) && !defined(WEB_CONFIGURATION)) || \
    (defined(WEB_COMMAND_SERVER) && defined(WEB_CONFIGURATION))
#  error "Faulty constraint" // see main.h
#endif

#include "command.h"
#include "config.h"
#include "log.h"
#include "network.h"
#include "util.h"

#include <WiFi.h>
#include <WiFiClient.h>

// This is extended for WEB_CONFIGURATION or WEB_COMMAND_SERVER below.

struct WebServerState {
  WebClient* clients = nullptr;
  WiFiServer server;
  WebServerState(int port) : server(port) {}
};

// Parse the input until it is terminated.  If the input was complete, invoke the processing
// function to handle it.  If the input was incomplete and there isn't any more, invoke the
// failure function to signal this.
//
// The request is a number of CRLF-terminated lines followed by a blank CRLF-terminated line.
// This state machine attempts to implement that precisely.  It may be that a looser
// interpretation would be more resilient.

void WebInputTask::poll(WebClient* cl) {
  // Client can disconnect at any time
  while (cl->client.connected()) {
    // Bytes arrive now and then.
    while (cl->client.available()) {
      char c = cl->client.read();
      //log("Web: received %c\n", c);
      switch (cl->state) {
      case RequestParseState::TEXT:
        if (c == '\r') {
          cl->state = RequestParseState::CR;
        }
        break;
      case RequestParseState::CR:
        if (c == '\n') {
          cl->state = RequestParseState::CRLF;
        } else if (c == '\r') {
          cl->state = RequestParseState::CR;
        } else {
          cl->state = RequestParseState::TEXT;
        }
        break;
      case RequestParseState::CRLF:
        if (c == '\r') {
          cl->state = RequestParseState::CRLFCR;
        } else {
          cl->state = RequestParseState::TEXT;
        }
        break;
      case RequestParseState::CRLFCR:
        if (c == '\n') {
          cl->state = RequestParseState::CRLFCRLF;
          goto request_completed;
        } else if (c == '\r') {
          cl->state = RequestParseState::CR;
        } else {
          cl->state = RequestParseState::TEXT;
        }
        break;
      case RequestParseState::CRLFCRLF:
        // Should not happen
        cl->state = RequestParseState::TEXT;
        goto request_completed;
      }
      cl->request += c;
    }
    // Connected (probably) but input not available and not final state, come back later.
    return;
  }

request_completed:
  if (cl->state == RequestParseState::CRLFCRLF) {
    cl->process_request();
  } else {
    cl->failed_request();
  }
}

// This polls the server for new clients, and for each active client, polls for input.
// It then reaps any dead clients.

void WebInputTask::execute(SnappySenseData*) {
  if (state == nullptr) {
    if (!start()) {
      return;
    }
  }

  // Listen for incoming clients
  for (;;) {
    WiFiClient client = state->server.available();
    if (!client) {
      break;
    }
    log("Web server: Incoming request\n");
    WebClient* webclient = create_client(std::move(client));
    webclient->next = state->clients;
    state->clients = webclient;
  }

  // Obtain traffic and respond to requests.  I guess in principle this should
  // be two loops, where the outer loop runs as long as some work has been
  // performed, but it doesn't seem important.
  for ( WebClient* cl = state->clients; cl != nullptr; cl = cl->next ) {
    if (!cl->dead) {
      poll(cl);
    }
  }

  // Garbage collect the clients
  WebClient* curr = state->clients;
  WebClient* prev = nullptr;
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
      WebClient* next = curr->next;
      if (prev == nullptr) {
        state->clients = next;
      } else {
        prev->next = next;
      }
      delete curr;
      curr = next;
    }
  }
}

// TODO: These ifdefs are ugly and should really be handled differently.  They
// are the reason WEB_COMMAND_SERVER and WEB_CONFIGURATION conflict.

#ifdef WEB_COMMAND_SERVER
struct WebServerCommandState : public WebServerState {
  WiFiHolder server_holder;
  WebServerCommandState() : WebServerState(web_server_listen_port()) {}
};

bool WebInputTask::start() {
  auto* state = new WebServerCommandState();
  state->server_holder = connect_to_wifi();
  if (!state->server_holder.is_valid()) {
    // TODO: Does somebody need to know?
    log("Failed to bring up web server\n");
    delete state;
    return false;
  }
  state->server.begin();
  log("Web server: listening on port %d\n", web_server_listen_port());
  this->state = state;
  return true;
}
#endif

#ifdef WEB_CONFIGURATION
struct WebServerConfigState : public WebServerState {
  // Port 80 makes the configuration UX better than for a random port.
  WebServerConfigState() : WebServerState(80) {}
};

bool WebInputTask::start() {
  const char* ssid = web_config_access_point();
  if (*ssid == 0) {
    render_text("Configuration mode\nNo cfg access point");
    return false;
  }
  // TODO: Handle return code!  If we fail to bring up the AP then we probably
  // should not try again, as we're wasting energy.  On the other hand, if we fail
  // to bring up the AP then there's probably a serious error, so maybe a panic is
  // the appropriate response...
  WiFi.softAP(ssid);  // No password
  IPAddress IP = WiFi.softAPIP();
  log("Soft AP SSID %s, IP address: %s\n", ssid, IP.toString().c_str());
  String msg = fmt("Configuration mode\n%s\n%s", ssid, IP.toString().c_str());
  render_text(msg.c_str());
  state = new WebServerConfigState();
  state->server.begin();
  log("Web server: listening on port 80\n");
  return true;
}
#endif

#endif // WEB_SERVER
