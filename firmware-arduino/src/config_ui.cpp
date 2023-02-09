#include "config_ui.h"
#include "config.h"
#include "device.h"
#include "network.h"
#include "web_server.h"

#ifdef WEB_CONFIGURATION

// Configuration file format version, see the 'config' statement help text further down.
// This is intended to be used in a proper "semantic versioning" way.
//
// Version 1.1:
//   Added web-config-access-point

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define BUGFIX_VERSION 0

// evaluate_config() evaluates a configuration program, using the `read_line` parameter
// to read lines of input from some source of text.  It returns a String, which is empty
// if everything was fine and is otherwise an error message.
//
// The configuration language is described by the manual below.

static String evaluate_config(List<String>& input) {
  for (;;) {
    if (input.is_empty()) {
      return fmt("Configuration program did not end with `end`");
    }
    String line = input.pop_front();
    String kwd = get_word(line, 0);
    if (kwd == "end") {
      return String();
    } else if (kwd == "clear") {
      reset_configuration();
    } else if (kwd == "save") {
      save_configuration();
    } else if (kwd == "version") {
      int major, minor, bugfix;
      if (sscanf(get_word(line, 1).c_str(), "%d.%d.%d", &major, &minor, &bugfix) != 3) {
        return fmt("Bad statement [%s]\n", line.c_str());
      }
      if (major != MAJOR_VERSION || (major == MAJOR_VERSION && minor > MINOR_VERSION)) {
        // Ignore the bugfix version for now, but require it in the input
        return fmt("Bad version %d.%d.%d, I'm %d.%d.%d\n",
                   major, minor, bugfix,
                   MAJOR_VERSION, MINOR_VERSION, BUGFIX_VERSION);
      }
      // version is OK
    } else if (kwd == "set") {
      String varname = get_word(line, 1);
      if (varname == "") {
        return fmt("Missing variable name for 'set'\n");
      }
      // It's legal to use "" as a value, but illegal not to have a value.
      bool flag = false;
      String value = get_word(line, 2, &flag);
      if (!flag) {
        return fmt("Missing value for variable [%s]\n", varname.c_str());
      }
      Pref* p = get_pref(varname.c_str());
      if (p == nullptr || p->is_cert()) {
        return fmt("Unknown or inappropriate variable name for 'set': [%s]\n", varname.c_str());
      }
      if (p->is_string()) {
        p->str_value = value;
      } else {
        p->int_value = int(value.toInt());
      }
    } else if (kwd == "cert") {
      String varname = get_word(line, 1);
      if (varname == "") {
        return fmt("Missing variable name for 'cert'\n");
      }
      String value;
      if (input.is_empty()) {
        return fmt("Unexpected end of input in config (certificate)");
      }
      String line = input.pop_front();
      if (!line.startsWith("-----BEGIN ")) {
        return fmt("Expected -----BEGIN at the beginning of cert");
      }
      value = line;
      value += "\n";
      for (;;) {
        if (input.is_empty()) {
          return fmt("Unexpected end of input in config (certificate)");
        }
        String line = input.pop_front();
        value += line;
        value += "\n";
        if (line.startsWith("-----END ")) {
          break;
        }
      }
      value.trim();
      Pref* p = get_pref(varname.c_str());
      if (p == nullptr || !p->is_cert()) {
        return fmt("Unknown or inappropriate variable name for 'cert': [%s]\n", varname.c_str());
      }
      p->str_value = value;
    } else {
      int i = 0;
      while (i < line.length() && isspace(line[i])) {
        i++;
      }
      if (i < line.length() && line[i] == '#') {
        // comment, do nothing
      } else if (i == line.length()) {
        // blank, do nothing
      } else {
        return fmt("Bad configuration statement [%s]\n", line.c_str());
      }
    }
  }
  /*NOTREACHED*/
  panic("Unreachable state in evaluate_config");
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
          <tr> <td>Location</td> <td><input name=location type="text" value="%s"/></td> <td></td> </tr>
        </table>
        <button>Submit</button>
      </form>
    </div>
  </body>
</html>
)EOF";

bool WebConfigRequestHandler::handle_get_user_config() {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.printf(config_page, "",
                access_point_ssid(1), access_point_password(1),
                access_point_ssid(2), access_point_password(2),
                access_point_ssid(3), access_point_password(3),
                location_name());
  return true;
}

bool WebConfigRequestHandler::handle_post_user_config(const char* buf) {
  bool updated = false;
  bool failed = true;
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
    if (key == "location") {
      set_location_name(value.c_str());
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
  if (!failed) {
    client.println("HTTP/1.1 202 Accepted");
    client.println("Content-type:text/html");
    client.println();
    client.printf(config_page, "VALUES UPDATED!",
                  access_point_ssid(1), access_point_password(1),
                  access_point_ssid(2), access_point_password(2),
                  access_point_ssid(3), access_point_password(3),
                  location_name());
  }
  return !failed;
}

String WebConfigRequestHandler::handle_post_factory_config(const char* buf) {
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
  return evaluate_config(lines);
}

bool WebConfigRequestHandler::handle_show_factory_config() {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  show_configuration(&client);
  return true;
}

char* WebConfigRequestHandler::get_post_data() {
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

void WebConfigRequestHandler::process_request() {
  bool failed = false;
  String errmsg;
  if (request.startsWith("GET / ")) {
    failed = !handle_get_user_config();
  } else if (request.startsWith("GET /show ")) {
    failed = !handle_show_factory_config();
  } else if (request.startsWith("POST /")) {
    char* post_data = get_post_data();
    if (post_data) {
      if (request.startsWith("POST / ")) {
        failed = !handle_post_user_config(post_data);
      } else if (request.startsWith("POST /config ")) {
        errmsg = handle_post_factory_config(post_data);
        failed = !errmsg.isEmpty();
      } else {
        failed = true;
      }
    } else {
      failed = true;
    }
    free(post_data);
  }
  if (failed) {
    log("Web server: invalid method or URL\n");
    if (!errmsg.isEmpty()) {
      log("Error: %s\n", errmsg.c_str());
    }
    client.println("HTTP/1.1 405 Bad request");
  }
  dead = true;
}

void WebConfigRequestHandler::failed_request() {
  log("Web server: Incomplete request [%s]\n", request.c_str());
  dead = true;
}

bool WebConfigTask::start() {
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
  if (!create_wifi_soft_access_point(ssid, nullptr, &ip)) {
    render_text("AP config failed.\n\nHanging now.");
    for(;;) {}
  }
  String msg = fmt("%s\n\n%s", ssid, ip.toString().c_str());
  render_text(msg.c_str());
  int port = 80;
  web_server = new WebServer(port);
  web_server->server.begin();
  log("Web server: listening on port %d\n", port);
  return true;
}
#endif // WEB_CONFIGURATION
