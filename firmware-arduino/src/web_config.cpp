// Logic for setting up a configuration access point and handling web requests received
// by a server on that access point.

#include "web_config.h"

#ifdef SNAPPY_WEBCONFIG

#include "config.h"
#include "device.h"
#include "network.h"
#include "web_server.h"

bool webcfg_start_access_point() {
  char buf[32];
  const char* ssid = web_config_access_point();
  if (*ssid == 0) {
    // No AP configured, but we're not defeated that easily.  Generate a randomish name.
    unsigned hibits = rand() % 65536;
    unsigned lobits = rand() % 65536;
    sprintf(buf, "snp_%04x_%04x_cfg", hibits, lobits);
    ssid = buf;
  }
  // TODO: Handle return code better!  If we fail to bring up the AP then we probably
  // should not try again, as we're wasting energy.  On the other hand, if we fail
  // to bring up the AP then there's probably a serious error, so maybe a panic is
  // the appropriate response...
  IPAddress ip;
  if (!wifi_create_access_point(ssid, nullptr, &ip)) {
    render_text("AP config failed.\n\nHanging now.");
    for(;;) {}
  }
  String msg = fmt("%s\n\n%s", ssid, ip.toString().c_str());
  render_text(msg.c_str());
  return true;
}

// Configuration / provisioning:
//
// The device acts as an access point, and its SSID and IP address are displayed on the
// device screen when the device is in configuration mode.  The user's device connects
// to this SSID, and loads the top-level page from the IP / port 80 - that is, just typing
// the IP address into the browser address bar is enough, no port or path is needed.
// This action brings up a configuration form, which can be filled in and submitted to store
// the values; the process can be repeated.

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
        </table>
        <button>Submit</button>
      </form>
    </div>
  </body>
</html>
)EOF";

static void handle_get_user_config(Stream& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.printf(config_page, "",
                access_point_ssid(1), access_point_password(1),
                access_point_ssid(2), access_point_password(2),
                access_point_ssid(3), access_point_password(3));
}

static void handle_post_user_config(Stream& client, const char* buf) {
  bool updated = false;
  bool failed = false;
  const char* p = (char*)buf;
  for (;;) {
    String key, value;
    int len;
    char id;
    if (failed) {
      break;
    }
    if (!get_posted_field(&p, &key, &value)) {
      break;
    }
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
    failed = true;
  }
  if (!failed && *p) {
    log("Stuff left in buffer: [%s]\n", p);
    failed = true;
  }
  if (!failed && updated) {
    save_configuration();
  }
  if (failed) {
    log("Bad request from client - unexpected field %s\n", p);
    client.println("HTTP/1.1 405 Bad request - unexpected field");
  } else {
    client.println("HTTP/1.1 202 Accepted");
    client.println("Content-type:text/html");
    client.println();
    client.printf(config_page, "VALUES UPDATED!",
                  access_point_ssid(1), access_point_password(1),
                  access_point_ssid(2), access_point_password(2),
                  access_point_ssid(3), access_point_password(3));
  }
}

static void handle_post_factory_config(Stream& client, const char* buf) {
  // In this case the content is a config script.  Split it into lines and hand it to the
  // config script processor.
  List<String> lines;
  const char* p = buf;
  while (*p) {
    String s;
    while (*p && *p != '\n') {
      s += *p++;
    }
    if (*p) {
      p++;
    }
    lines.add_back(std::move(s));
  }
  int bad_line;
  String msg;
  bool was_saved;
  String err = evaluate_configuration(lines, &was_saved, &bad_line, &msg);
  if (err.isEmpty()) {
    client.println("HTTP/1.1 200 OK");
    String buf("Config accepted");
    if (was_saved) {
      buf += "\n\nConfig saved";
    } else {
      buf += "\n\n*** NOT SAVED ***";
    }
    render_text(buf.c_str());
  } else {
    String buf("HTTP/1.1 405 Invalid config ");
    buf += err;
    client.println(buf.c_str());
    log("Web server: invalid factory config: %s\n", err.c_str());
    render_text(fmt("Bad config\nLine %d\n%s", bad_line, msg.c_str()).c_str());
  }
}

static void handle_show_factory_config(Stream& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  show_configuration(&client);
}

char* get_post_data(Stream& client, const String& request) {
  int ix = request.indexOf("Content-Length:");
  if (ix == -1) {
    return nullptr;
  }
  int bufsiz;
  if (sscanf(request.c_str()+ix+15, "%d", &bufsiz) != 1) {
    return nullptr;
  }
  uint8_t* buf = (uint8_t*)malloc(bufsiz+1);
  if (buf == nullptr) {
    return nullptr;
  }
  size_t bytes_read = client.readBytes(buf, bufsiz);
  buf[bytes_read] = 0;
  return (char*)buf;
}

void webcfg_process_request(Stream& client, const String& request) {
  bool bad_request = false;
  // The request handler performs all processing:
  // - it replies to the client, also on error
  // - it logs, if logging is required
  // - it updates the display, if display updating is required
  if (request.startsWith("GET / ")) {
    handle_get_user_config(client);
  } else if (request.startsWith("GET /show ")) {
    handle_show_factory_config(client);
  } else if (request.startsWith("POST /")) {
    char* post_data = get_post_data(client, request);
    if (post_data) {
      if (request.startsWith("POST / ")) {
        handle_post_user_config(client, post_data);
      } else if (request.startsWith("POST /config ")) {
        handle_post_factory_config(client, post_data);
      } else {
        bad_request = true;
      }
    } else {
      bad_request = true;
    }
    free(post_data);
  } else {
    bad_request = true;
  }
  if (bad_request) {
    client.println("HTTP/1.1 405 Bad request");
    log("Web server: invalid method or URL %s\n", request.c_str());
  }
}

void webcfg_failed_request(Stream& client, const String& request) {
  client.println("HTTP/1.1 405 Bad request");
  log("Web server: Incomplete request [%s]\n", request.c_str());
}

#endif // SNAPPY_WEBCONFIG
