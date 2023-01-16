// Support for the device acting as a simple web server, for querying data across HTTP
//
// Parameters are passed using the usual syntax, so <http://.../get?temperature> to
// return the temperature reading.  The parameter handling is ad-hoc and works only
// for these simple cases.

#include "web_server.h"

#ifdef WEB_SERVER

#include "command.h"
#include "config.h"
#include "log.h"
#include "network.h"

#include <WiFi.h>
#include <WiFiClient.h>

/* Server/Client wifi state machine is basically similar to the Unix stack:
 *  WiFi.begin() connects the system to a local access point.
 *  Then, create a WiFiServer `server` to handle incoming connections on a port.
 *  server.begin() is bind() + listen(): it starts listening on a port, but does not block.
 *  server.available() is accept() + client socket setup, but it will not block: if there is no client,
 *       available() returns a falsy client.  Be sure not to spin...  A callback would have been better.
 *       Anyway, I can't tell if the server blocks subsequent connects until we've accepted,
 *       or overwrites one connect with a new one, but perhaps it doesn't matter.
 *  client.connected() can become false at any time if the client disconnects.
 *  client.available() says there's data.  Again, would be better to get a callback.
 *  client.stop() closes the client socket after use
 *  server.close() closes the server socket and stops listening
 */

// I suppose it would be possible to use the Arduino WEB_SERVER framework?

enum ClientState {
  TEXT,
  CR,
  CRLF,
  CRLFCR,
  CRLFCRLF,
};

struct WebClientState {
  // FIXME: Use List<>
  WebClientState* next = nullptr;
  ClientState state = TEXT;
  String request;
  bool dead = false;
  WiFiClient client;
  WebClientState(WiFiClient&& client) : client(std::move(client)) {}
  void process_request();
};

struct WebServerState {
  WebClientState* clients = nullptr;
  WiFiServer server;
  WiFiHolder server_holder;
  WebServerState() : server(web_server_listen_port()) {}
};

void ReadWebInputTask::start() {
  state = new WebServerState();
  state->server_holder = connect_to_wifi();
  if (!state->server_holder.is_valid()) {
    // TODO: Does somebody need to know?
    log("Failed to bring up web server\n");
    delete state;
    return;
  }
  state->server.begin();
  log("Web server: listening on port %d\n", web_server_listen_port());
}

void ReadWebInputTask::execute(SnappySenseData*) {
  if (state == nullptr) {
    start();
  }

  // Listen for incoming clients
  for (;;) {
    WiFiClient client = state->server.available();
    if (!client) {
      break;
    }
    log("Web server: Incoming request\n");
    WebClientState* webclient = new WebClientState(std::move(client));
    webclient->next = state->clients;
    state->clients = webclient;
  }

  // Obtain traffic and respond to requests.  I guess in principle this should
  // be two loops, where the outer loop runs as long as some work has been
  // performed, but it doesn't seem important.
  for ( WebClientState* cl = state->clients; cl != nullptr; cl = cl->next ) {
    poll(cl);
  }

  // Garbage collect the clients
  WebClientState* curr = state->clients;
  WebClientState* prev = nullptr;
  while (curr != nullptr) {
    if (curr->dead) {
      curr->client.flush();
      curr->client.stop();
      WebClientState* next = curr->next;
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

void ReadWebInputTask::poll(WebClientState* cl) {
  // Client can disconnect at any time
  while (cl->client.connected()) {
    // Bytes arrive now and then.
    // The request is a number of CRLF-terminated lines followed by a blank CRLF-terminated line.
    // This state machine attempts to implement that precisely.  It may be that a looser 
    // interpretation would be more resilient.
    while (cl->client.available()) {
      char c = cl->client.read();
      //log("Web: received %c\n", c);
      switch (cl->state) {
      case TEXT:
        if (c == '\r') {
          cl->state = CR;
        }
        break;
      case CR:
        if (c == '\n') {
          cl->state = CRLF;
        } else if (c == '\r') {
          cl->state = CR;
        } else {
          cl->state = TEXT;
        }
        break;
      case CRLF:
        if (c == '\r') {
          cl->state = CRLFCR;
        } else {
          cl->state = TEXT;
        }
        break;
      case CRLFCR:
        if (c == '\n') {
          cl->state = CRLFCRLF;
        } else if (c == '\r') {
          cl->state = CR;
        } else {
          cl->state = TEXT;
        }
        break;
      case CRLFCRLF:
        // Shouldn't happen.
        cl->state = TEXT;
        goto exit;
        break;
      }
      cl->request += c;
      if (cl->state == CRLFCRLF) {
        break;
      }
    }

    // Connected but input not available, come back later
    return;
  }

  exit:
  if (cl->state == CRLFCRLF) {
    cl->process_request();
  } else {
    log("Web server: Incomplete request [%s]\n", cl->request.c_str());
    cl->dead = true;
  }
}

// This keeps the client alive until the command task has provided
// output.
class ProcessCommandTaskFromWeb : public ProcessCommandTask {
  WebClientState* cl;
public:
  ProcessCommandTaskFromWeb(WebClientState* cl) : ProcessCommandTask(cl->request, &cl->client), cl(cl) {}
  ~ProcessCommandTaskFromWeb() {
    cl->dead = true;
  }
};

void WebClientState::process_request() {
  if (request.startsWith("GET /")) {
    log("Web server: handling GET\n");
    String r = request.substring(5);
    int idx = r.indexOf(' ');
    if (idx != -1) {
      r = r.substring(0, idx);
      // TODO: Issue 24: technically this is url-encoded and needs to be decoded
    }
    if (r.isEmpty()) {
      r = String("help");
    }
    // Hack
    idx = r.indexOf('?');
    if (idx != -1) {
      r[idx] = ' ';
    }
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.println("<pre>");
    sched_microtask_after(new ProcessCommandTaskFromWeb(this), 0);
    client.println("</pre>");
    return;
  }
  log("Web server: invalid method\n");
  client.println("HTTP/1.1 403 Forbidden");
}

#endif // WEB_SERVER
