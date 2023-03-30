/* Send data over i2c to a slave device.
 *
 * This is intended to be used from a Raspberry Pi with I2C SCL/SDA/GND
 * connected to the slave device.
 *
 * Usage:
 *   i2c-send [--addr addr] [--dev device] [--verbose | -v]
 *
 * `addr`
 *   The i2c slave device's address.  It defaults to 0x28 (for historical reasons).
 *   The address can be specified in decimal or hex.
 *
 * `device`
 *   The device node.  It defaults to /dev/i2c-1.
 *
 * Input is read from stdin and is sent verbatim to the device.  Errors are written to stdout.
 *
 * TODO:
 *  - option to prefix by eg byte count (requires file input, or buffering)
 *  - option to checksum and send checksum at end
 *  - possibly we should not have a general program here but something specialized to snappysense
 */

/* Some ideas here from pigpio.c, see https://github.com/joan2937/pigpio */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

static int remote_addr = 0x28;
static const char* device = "/dev/i2c-1";
static int verbose = 0;

int main(int argc, char** argv) {
  char buf[1024];
  int fd = open(device, O_RDWR);
  if (fd < 0) {
    sprintf(buf, "Open %s", device);
    perror(buf);
    return 1;
  }

  /* This call to I2C_SLAVE comes from pigpio.c.  It could appear that
   * if we set the remote address here then we can write to the remote
   * device using a normal write() on the fd.  See eg
   * https://www.waveshare.com/wiki/Raspberry_Pi_Tutorial_Series:_I2C#Control_by_sysfs
   * https://github.com/torvalds/linux/blob/master/drivers/i2c/i2c-dev.c#L118
   */
  /*
  if (ioctl(fd, I2C_SLAVE, (unsigned long)addr) < 0) {
    perror("set address on /dev/i2c-1");
    return 1;
  }
  */

  unsigned long funcs;
  if (ioctl(fd, I2C_FUNCS, &funcs) < 0) {
    sprintf(buf, "Get functions from %s", device);
    perror(buf);
    return 1;
  }
  if (verbose) {
    printf("Device capabilities: %08lx\n", funcs);
  }

  /* Construct an outgoing message that sends 0x05 10 times, in
     one message */
  char payload[] = {'H','E','L','L','O','\n'};
  struct i2c_msg msgs[1];
  msgs[0].addr = remote_addr;
  msgs[0].flags = 0;		/* Write */
  msgs[0].len = sizeof(payload);
  msgs[0].buf = payload;
  struct i2c_rdwr_ioctl_data data;
  data.msgs = msgs;
  data.nmsgs = 1;
  if (ioctl(fd, I2C_RDWR, &data) < 0) {
    sprintf(buf, "write %s", device);
    perror(buf);
    return 1;
  }

  close(fd);
  return 0;
}

  
