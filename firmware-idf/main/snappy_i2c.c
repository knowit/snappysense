#include "snappy_i2c.h"

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"

bool snappy_i2c_probe(unsigned i2c_bus, unsigned address) {
  uint8_t dummy = 0;
  return snappy_i2c_write(i2c_bus, address,
			  &dummy, /* nbytes= */ 0,
			  /* timeoutMs= */ 200);
}

static bool snappy_i2c_primitive_write(unsigned i2c_bus, unsigned address, unsigned prefix,
				       bool has_prefix, const void* buffer, size_t nbytes,
				       unsigned timeoutMs) {
  /* Body of this mostly copied from Arduino code (i2cWrite), Apache license */
  /* The size of the cmd_buff is highly questionable */
  /* TODO: Avoid stack-allocating cmd_buff */
#define NUM_TRANSACTIONS 5
  uint8_t cmd_buff[I2C_LINK_RECOMMENDED_SIZE(NUM_TRANSACTIONS)] = { 0 };
  i2c_cmd_handle_t cmd = i2c_cmd_link_create_static(cmd_buff, I2C_LINK_RECOMMENDED_SIZE(NUM_TRANSACTIONS));
#undef NUM_TRANSACTIONS
  esp_err_t ret = i2c_master_start(cmd);
  if (ret != ESP_OK) {
    goto end;
  }
  ret = i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
  if (ret != ESP_OK) {
    printf("@1\n");
    goto end;
  }
  if (has_prefix) {
    uint8_t tmp = prefix;
    ret = i2c_master_write(cmd, &tmp, 1, true);
    if (ret != ESP_OK) {
      printf("@2\n");
      goto end;
    }
  }
  if(nbytes){
    ret = i2c_master_write(cmd, buffer, nbytes, true);
    if (ret != ESP_OK) {
      printf("@3\n");
      goto end;
    }
  }
  ret = i2c_master_stop(cmd);
  if (ret != ESP_OK) {
    printf("@4\n");
    goto end;
  }
  ret = i2c_master_cmd_begin((i2c_port_t)i2c_bus, cmd, timeoutMs / portTICK_PERIOD_MS);
  if (ret != ESP_OK) {
    printf("@5 %d\n", ret);
  }

 end:
  if(cmd != NULL){
    i2c_cmd_link_delete_static(cmd);
  }
  return ret == ESP_OK;
}

bool snappy_i2c_write_prefixed(unsigned i2c_bus, unsigned address, unsigned prefix,
			       const void* buffer, size_t nbytes, unsigned timeoutMs) {
  return snappy_i2c_primitive_write(i2c_bus, address, prefix, true, buffer, nbytes, timeoutMs);
}

bool snappy_i2c_write(unsigned i2c_bus, unsigned address, const void* buffer, size_t nbytes,
		      unsigned timeoutMs) {
  return snappy_i2c_primitive_write(i2c_bus, address, 0, false, buffer, nbytes, timeoutMs);
}

bool snappy_i2c_read(unsigned i2c_bus, unsigned address, uint8_t* buff, size_t size,
		     unsigned timeOutMillis, size_t *readCount) {
  /* Body of this mostly copied from Arduino code (i2cREad), Apache license */
  esp_err_t ret = ESP_FAIL;
  ret = i2c_master_read_from_device((i2c_port_t)i2c_bus, address, buff, size, timeOutMillis / portTICK_PERIOD_MS);
  if(ret == ESP_OK){
    *readCount = size;
  } else {
    *readCount = 0;
  }
  return ret == ESP_OK;
}

/* Old code */

/* Look for i2c devices. */
#if 0
  for ( int address=1 ; address < 127; address++ ) {
    if (snappy_i2c_probe(i2c_master_port, address)) {
      printf("Found device @ %02x\n", address);
    }
  }
#endif

