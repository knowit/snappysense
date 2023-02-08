/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "factory_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

void factory_configuration() {
  vTaskDelay(pdMS_TO_TICKS(2000));
  show_text("Factory config\nUnimplemented\nPress reset");
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
