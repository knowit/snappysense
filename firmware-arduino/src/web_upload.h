// Code for uploading data to a remote HTTP server.
//
// The remote server must know how to handle POST to /data with a JSON object payload.
// For a simple server that can do this, see `test-server/` in the present repo.

#ifndef web_upload_h_included
#define web_upload_h_included

#include "main.h"

// If we're going to have both web upload and mqtt upload then there is probably
// a joint "upload manager" to keep track of the data to be uploaded, to expire
// that data when necessary, to discard it if the device is or becomes disabled,
// and so on.  Those bits need not be duplicated in each of the upload pipes.
// On the other hand, this ties the upload cadence for each to the other, and that
// is not all that useful.
//
// What's the value of the web upload now that we have mqtt?  The web upload can't
// do device commands (well, there's POST and GET I guess that can be integrated
// somehow to do polling, but why).  It would be better to instead generalize the
// mqtt code beyond AWS so that it could upload to a private mqtt broker.
//
// So leave the web upload code alone for the time being without fixing it, but
// try to get some functionality working for contacting mosquitto or some other
// client on a suitable host:
//
//   - pref bit saying how to contact the mqtt service (basically the client setup)
//   - possibly the ability to store multiple sets of mqtt credentials
//   - maybe there's a table of credentials and the pref bit is an index into that table

#ifdef WEB_UPLOAD

#include "sensor.h"

class WebUploadTask final : public MicroTask {
public:
  const char* name() override {
    return "Web upload";
  }
  virtual bool only_when_device_enabled() {
    return true;
  }
  void execute(SnappySenseData*) override;
};

#endif

#endif // !web_upload_h_included
