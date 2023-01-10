// Code for uploading data to a remote HTTP server.
//
// The remote server must know how to handle POST to /data with a JSON object payload.
// For a simple server that can do this, see `../server`.

#ifndef web_upload_h_included
#define web_upload_h_included

#include "main.h"

#ifdef WEB_UPLOAD

#include "microtask.h"
#include "sensor.h"

class WebUploadTask final : public MicroTask {
public:
  const char* name() override {
    return "Web upload";
  }
  void execute(SnappySenseData*) override;
};

#endif

#endif // !web_upload_h_included
