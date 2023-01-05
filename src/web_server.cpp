// Support for the device acting as a simple web server, for querying data across HTTP

#include "web_server.h"
#include "config.h"
#include "log.h"
#include "network.h"

#include <WiFi.h>
#include <WiFiClient.h>

#ifdef WEB_SERVER
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

// I suppose it would be possible to use the Arduino WEB_SERVER framework, but it insists on
// handling its own WiFiClient and I don't like it.

WiFiServer server(web_server_listen_port());

void start_web_server() {
  connect_to_wifi();

  server.begin();
  log("Web server is listening on port %d\n", web_server_listen_port());
}

static void handle_web_request(const SnappySenseData& data, WiFiClient& client, const String& request) {
  if (request.startsWith("GET / ")) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.println("<div>");
    client.println(" <div>Valid requests:</div>");
    for ( SnappyMetaDatum* r = snappy_metadata; r->json_key != nullptr ; r++ ) {
        client.printf(" <div><a href=\"%s\">%s</a></div>\n", r->json_key, r->explanatory_text);
    }
    client.println("</div>");
    return;
  }
  char buf[256];
  for ( SnappyMetaDatum* r = snappy_metadata; r->json_key != nullptr ; r++ ) {
    sprintf(buf, "GET /%s ", r->json_key);
    if (request.startsWith(buf)) {
        r->format(data, buf, buf+sizeof(buf));
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        client.printf("<div>%s: %s</div>\n", r->explanatory_text, buf);
        return;
    }
  }
  client.println("HTTP/1.1 404 Bad request");
  client.println("Content-type:text/html");
  client.println();
}

void maybe_handle_web_request(const SnappySenseData& data) {
  WiFiClient client = server.available();   // listen for incoming clients
  if (!client) {
    return;
  }
#if 0
  log("Got a client");
#endif
  // The request is a number of CRLF-terminated lines followed by a blank CRLF-terminated line.
  // This state machine attempts to implement that precisely.  It may be that a looser 
  // interpretation would be more resilient.
  enum {
    TEXT,
    CR,
    CRLF,
    CRLFCR,
    CRLFCRLF,
  } state = TEXT;
  String request = "";
  // Client can disconnect at any time
  while (client.connected()) {
    // Bytes arrive now and then
    if (client.available()) {
      char c = client.read();
#if 0
      log("Received %c\n", c);
#endif
      switch (state) {
      case TEXT:
        if (c == '\r') {
          state = CR;
        }
        break;
      case CR:
        if (c == '\n') {
          state = CRLF;
        } else if (c == '\r') {
          state = CR;
        } else {
          state = TEXT;
        }
        break;
      case CRLF:
        if (c == '\r') {
          state = CRLFCR;
        } else {
          state = TEXT;
        }
        break;
      case CRLFCR:
        if (c == '\n') {
          state = CRLFCRLF;
        } else if (c == '\r') {
          state = CR;
        } else {
          state = TEXT;
        }
        break;
      case CRLFCRLF:
        // Shouldn't happen.
        state = TEXT;
        goto bad;
        break;
      }
      request += c;
      if (state == CRLFCRLF) {
        break;
      }
    } else {
      delay(1);
    }
  }
  bad:
  if (state == CRLFCRLF) {
    handle_web_request(data, client, request);
  } else {
    log("Incomplete request");
  }
  client.flush();
  client.stop();
}

#endif // WEB_SERVER

