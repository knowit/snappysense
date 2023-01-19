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

// One object for each client that has connected to the server.
class WebClient {
public:
  // These fields are PRIVATE to the web server framework

  // List of clients attached to some server
  WebClient* next = nullptr;

  // Input parsing state
  RequestParseState state = RequestParseState::TEXT;

  // The WiFi client for this web client
  WiFiClient client;

public:
  // These properties and methods are for the client.

  WebClient(WiFiClient&& client) : client(std::move(client)) {}
  virtual ~WebClient() {}

  // This is the request that has been collected for processing.
  String request;

  // process_request() and failed_request() set `dead` to true when the client is done.
  bool dead = false;

  // Invoked when a complete request has been seen.
  virtual void process_request() = 0;

  // Invoked when an incomplete request has been seen and we know there's no more input.
  virtual void failed_request() = 0;
};

class WebInputTask : public MicroTask {
  WebServerState* state = nullptr;
  bool start();
  void poll(WebClient*);

public:
  virtual WebClient* create_client(WiFiClient&& client) = 0;

  void execute(SnappySenseData*) override;
};

#endif // WEB_SERVER

#endif // web_server_h_included
