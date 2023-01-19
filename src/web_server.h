// Support for the device acting as a simple web server, for configuration and commands

#ifndef web_server_h_included
#define web_server_h_included

#include "main.h"

#ifdef WEB_SERVER
#include "sensor.h"

#include <WiFi.h>
#include <WiFiClient.h>

struct WebServerState;

// HTTP request parsing state.

enum class RequestParseState {
  TEXT,
  CR,
  CRLF,
  CRLFCR,
  CRLFCRLF,
};

// One request handler object for each connection created to a server.

class WebRequestHandler {
public:
  // These fields are PRIVATE to the web server framework

  // List of clients attached to some server
  WebRequestHandler* next = nullptr;

  // Input parsing state
  RequestParseState state = RequestParseState::TEXT;

  // The WiFi client for this web client
  WiFiClient client;

public:
  // These properties and methods are for the client.

  WebRequestHandler(WiFiClient&& client) : client(std::move(client)) {}
  virtual ~WebRequestHandler() {}

  // This is the request that has been collected for processing.
  String request;

  // process_request() and failed_request() set `dead` to true when the client is done.
  bool dead = false;

  // Invoked when a complete request has been seen.
  virtual void process_request() = 0;

  // Invoked when an incomplete request has been seen and we know there's no more input.
  virtual void failed_request() = 0;
};

// This holds an active server and a list of its clients.  It can be subclassed to add
// data specific to the connection (for example, for the command server the wifi connection
// is kept alive by this object).

struct WebServer {
  // FIXME: These are private to the server framework
  WebRequestHandler* request_handlers = nullptr;
  WiFiServer server;

  WebServer(int port) : server(port) {}
  virtual ~WebServer() {}
};

// An abstract web server task, not a great abstraction.

class WebInputTask : public MicroTask {
  void poll(WebRequestHandler*);

public:
  virtual ~WebInputTask() {}

  // This member is managed by the concrete class, not by this class.  Specifically,
  // if it needs to be deleted, somebody else needs to do it.
  WebServer* web_server = nullptr;

  // Create a WebRequestHandler of the appropriate type on behalf of the input task type.
  virtual WebRequestHandler* create_request_handler(WiFiClient&& client) = 0;

  // Create the necessary web server and other connections.  (For example, a task
  // can connect to a network and listen on a port; or can create a soft AP and
  // listen on a port on that AP.)
  virtual bool start() = 0;

  void execute(SnappySenseData*) override;
};

#endif // WEB_SERVER

#endif // web_server_h_included
