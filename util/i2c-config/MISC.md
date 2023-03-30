-*- fill-column: 100 -*-

# Misc RPi notes

## Raspberry Pi 2 B+

(This is the newest Pi I have)

https://www.raspberrypi.com/documentation/computers/processors.html#bcm2836

About the BCM2836: "The Broadcom chip used in the Raspberry Pi 2 Model B. The underlying
architecture in BCM2836 is identical to BCM2835. The only significant difference is the removal of
the ARM1176JZF-S processor and replacement with a quad-core Cortex-A7 cluster."

## I2C

On the BCM2835 and later there is extensive interrupt-driven buffered I2C support, and Linux has
drivers for this.  There is no need for any bit-banging.  Speeds up to 400Kbps are supported
("standard" I2C is 100Kbps), if the remote party plays ball.  I don't yet know how to configure
higher speed.

One possible limitation is that the I2C interface is described as single-master.  (I forget where I
saw this, but I think it relates to the Linux device driver.  A note I made to myself: "it behaves
like a master always, from what i can tell - it doesn't have an address that peripherals can
address, it looks like transfers always have to be initiated from the pi.  to get the effect of
being a slave the pi can initiate a master read however, and block on that.")

I2C shows up as /dev/i2c-* for various channels, i2c-1 is the primary pin2/pin3 interface, i2c-2 is
the secondary eeprom interface, I have not tested the latter.  These are char devices.  The devices
can be manipulated with ioctl as in i2c-test.c in this directory, and data sent and received using
that mechanism.

As they are char devices they can also be written to and read from with write() and read() provided
that they have been bound to remote addresses first.  The I2C_SLAVE ioctl operation can be used to
perform such binding from C, see i2c-test.c.  write() and read() are a little simpler than ioctl(),
so I guess this is nice.

More interestingly is whether address binding can be done from a shell script by writing stuff to
the device or device files in sysfs.  I haven't found a built-in mechanism yet; the new_device
mechanism in sysfs seems geared more toward bringing up drivers at run-time.  In pigpio
(https://abyz.me.uk/rpi/pigpio/), this problem is fixed by having a daemon act as the front,
programs need only talk to the daemon and can ignore the device level and don't need to be working
with ioctl.

For a practical application there would therefore be some exe that performs the ioctl to bind the
address and then a script could use read/write, in principle, to communicate.  Untested.

## Linux drivers

```
$ lsmod | grep i2c
i2c_bcm2835            16384  0
i2c_dev                20480  0
```

So it appears that the right driver is loaded.

However, a mystery: There is `/sys/bus/i2c/drivers/stmpe-i2c`, and what is this?  The directory has
three files, `bind`, `unbind`, and `uevent` (and it has a sibling, `dummy`).  This appears to be an
STM client driver, https://github.com/torvalds/linux/blob/master/drivers/mfd/stmpe-i2c.c.  Why is it
here?  It appears even if my STM board is not connected during boot.

It could look like it's a driver that's loaded because the STMPE is a popular range of extension
devices for the Pi.  This is juicy: https://forums.raspberrypi.com/viewtopic.php?t=148572.  These
devices all communicate over I2C and the driver is loaded to be available for them if they appear.

