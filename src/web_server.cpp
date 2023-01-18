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

// HTTP request parsing state.

enum RequestParseState {
  TEXT,
  CR,
  CRLF,
  CRLFCR,
  CRLFCRLF,
};

// One object for each client that has connected to the server.

struct WebClientState {
  // Use List<WebClientState*>?  For that, the list must be iterable and it must be possible
  // to remove items in the middle of it.
  WebClientState* next = nullptr;
  RequestParseState state = TEXT;
  String request;
  bool dead = false;
  WiFiClient client;
  WebClientState(WiFiClient&& client) : client(std::move(client)) {}
  void process_request();
  void failed_request();
};

// This is extended for WEB_CONFIGURATION or WEB_COMMAND_SERVER below.

struct WebServerState {
  WebClientState* clients = nullptr;
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

void ReadWebInputTask::poll(WebClientState* cl) {
  // Client can disconnect at any time
  while (cl->client.connected()) {
    // Bytes arrive now and then.
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
          goto request_completed;
        } else if (c == '\r') {
          cl->state = CR;
        } else {
          cl->state = TEXT;
        }
        break;
      case CRLFCRLF:
        // Should not happen
        cl->state = TEXT;
        goto request_completed;
      }
      cl->request += c;
    }
    // Connected (probably) but input not available and not final state, come back later.
    return;
  }

request_completed:
  if (cl->state == CRLFCRLF) {
    cl->process_request();
  } else {
    cl->failed_request();
  }
}

// This polls the server for new clients, and for each active client, polls for input.
// It then reaps any dead clients.

void ReadWebInputTask::execute(SnappySenseData*) {
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
    WebClientState* webclient = new WebClientState(std::move(client));
    webclient->next = state->clients;
    state->clients = webclient;
  }

  // Obtain traffic and respond to requests.  I guess in principle this should
  // be two loops, where the outer loop runs as long as some work has been
  // performed, but it doesn't seem important.
  for ( WebClientState* cl = state->clients; cl != nullptr; cl = cl->next ) {
    if (!cl->dead) {
      poll(cl);
    }
  }

  // Garbage collect the clients
  WebClientState* curr = state->clients;
  WebClientState* prev = nullptr;
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

#ifdef WEB_COMMAND_SERVER
struct WebServerCommandState : public WebServerState {
  WiFiHolder server_holder;
  WebServerCommandState() : WebServerState(web_server_listen_port()) {}
};

bool ReadWebInputTask::start() {
  state = new WebServerCommandState();
  state->server_holder = connect_to_wifi();
  if (!state->server_holder.is_valid()) {
    // TODO: Does somebody need to know?
    log("Failed to bring up web server\n");
    delete state;
    return false;
  }
  state->server.begin();
  log("Web server: listening on port %d\n", web_server_listen_port());
  return true;
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
  log("Processing %s\n", request.c_str());
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
  log("Web server: invalid method or URL\n");
  client.println("HTTP/1.1 403 Forbidden");
}
#endif

#ifdef WEB_CONFIGURATION
struct WebServerConfigState : public WebServerState {
  // Port 80 makes the configuration UX better than for a random port.
  WebServerConfigState() : WebServerState(80) {}
};

bool ReadWebInputTask::start() {
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

// This template has eight fields: message, ssid1, password1, ssid2, password2, ssid3, password3, location,
// which are filled in before the form is sent.  Since access to this page is currently http: and not
// https: the fields can be sniffed.  We hope the wifi traffic is encrypted.
//
// TODO! Must add enough CSS to make this legible somehow for phone screen.
static const char config_page[] = R"EOF(
<html>
  <head>
    <style>
      table { width: 100%%; font-size: 2em }
      input { font-size: 0.7em }
      button { font-size: 2em }
      .status { font-size: 2em }
    </style>
    <title>SnappySense configuration</title>
  </head>
  <body>
    <h1>SnappySense configuration</h1>
    <div class="status">%s&nbsp;</div>
    <div>&nbsp;</div>
    <div>
      <form method="POST" action="/">
        <table>
          <tr> <td>SSID1</td> <td><input name=ssid1 type="text" value="%s">&nbsp;</td>
            <td>Password</td> <td><input name=password1 type="text" value="%s"/></td></tr>
          <tr> <td>SSID2</td> <td><input name=ssid2 type="text" value="%s"/></td>
            <td>Password</td> <td><input name=password2 type="text" value="%s"/></td></tr>
          <tr> <td>SSID3</td> <td><input name=ssid3 type="text" value="%s"/></td>
            <td>Password</td> <td><input name=password3 type="text" value="%s"/></td></tr>
          <tr> <td>Location</td> <td><input name=location type="text" value="%s"/></td> <td></td> </tr>
        </table>
        <button>Submit</button>
      </form>
    </div>
  </body>
</html>
)EOF";

void WebClientState::process_request() {
  if (request.startsWith("GET / ")) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.printf(config_page, "",
                  access_point_ssid(1), access_point_password(1),
                  access_point_ssid(2), access_point_password(2),
                  access_point_ssid(3), access_point_password(3),
                  location_name());
    dead = true;
    return;
  }

  if (request.startsWith("POST / ")) {
    int ix = request.indexOf("Content-Length:");
    int bufsiz = 0;
    uint8_t* buf = nullptr;
    size_t bytes_read = 0;
    bool updated = false;
    const char* p = nullptr;
    if (ix == -1) {
      goto failure;
    }
    if (sscanf(request.c_str()+ix+15, "%d", &bufsiz) != 1) {
      goto failure;
    }
    buf = (uint8_t*)malloc(bufsiz+1);
    if (buf == nullptr) {
      goto failure;
    }
    bytes_read = client.readBytes(buf, bufsiz);
    buf[bytes_read] = 0;
    p = (char*)buf;
    for(;;) {
      String key, value;
      int len;
      char id;
      if (!get_posted_field(&p, &key, &value)) {
        break;
      }
      log("Field: %s %s\n", key.c_str(), value.c_str());
      if (sscanf(key.c_str(), "ssid%c%n", &id, &len) == 1 && len == 5) {
        set_access_point_ssid(id-'0', value.c_str());
        updated = true;
        continue;
      }
      if (sscanf(key.c_str(), "password%c%n", &id, &len) == 1 && len == 9) {
        set_access_point_password(id-'0', value.c_str());
        updated = true;
        continue;
      }
      if (key == "location") {
        set_location_name(value.c_str());
        updated = true;
        continue;
      }
      goto failure;
    }
    if (*p) {
      log("Stuff left in buffer: [%s]\n", p);
      goto failure;
    }
    if (updated) {
      save_configuration();
    }
    client.println("HTTP/1.1 202 Accepted");
    client.println("Content-type:text/html");
    client.println();
    client.printf(config_page, "VALUES UPDATED!",
                  access_point_ssid(1), access_point_password(1),
                  access_point_ssid(2), access_point_password(2),
                  access_point_ssid(3), access_point_password(3),
                  location_name());
    free(buf);
    buf = nullptr;
    dead = true;
    return;

  failure:
    log("Web server: bad request\n");
    if (buf != nullptr) {
      log(" [%s]\n", buf);
      free(buf);
    }
    client.println("HTTP/1.1 400 Bad request");
    dead = true;
    return;
  }

  log("Web server: invalid method or URL\n");
  client.println("HTTP/1.1 405 Bad request");
  dead = true;
}
#endif // WEB_CONFIGURATION

void WebClientState::failed_request() {
  log("Web server: Incomplete request [%s]\n", request.c_str());
  dead = true;
}

#endif // WEB_SERVER
