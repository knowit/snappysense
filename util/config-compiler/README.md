-*- fill-column: 100 -*-

# Configuration file compiler

(This is currently only intended for use with the IDF firmware.)

This program compiles a textual factory configuration file into a binary format.  The firmware
interprets only the binary format; the purpose of this indirection is to simplify the firmware,
making it smaller and more robust.  The firmware still must be able to handle malformed input and
out-of-range values, but it can be simplified by eg masking values to their assumed correct range
instead of checking for specific ranges.

There's a sample configuration file in `test.cfg`.

So why not just use JSON for the config file and decode it on the device, which has a JSON decoder
anyway to handle configuration commands from the server?  I hope to eventually be rid of both the
JSON encoder and decoder in the firmware.  For LoRa and other networks of that ilk, and for the very
smallest devices, we want to use binary encoding anyway.

## Input language

The input format is line-oriented.

First, leading and trailing whitespace on each line is stripped.

Next, empty lines and lines starting with `#` are discarded.

The remaining lines must comprise a sequence of Statements starting with
`snappysense-compiled-config` and ending with `end` or `save`.

### Statement `snappysense-compiled-config x.y.z`

From version 1.0.0.

The `x`, `y`, `z` are integers and the statement succeeds if the version stated can be understood by
the program reading the config (ie both the config compiler and the prefs parser on the device).

This statement must be first, and it must appear.

### Statement `clear`

From version 1.0.0.

Clear any saved configuration in RAM, ie, reset everything to default empty values.

If this appears it must be second, following the `snappysense-compiled-config`.

### Statement `set <variable> <value>`

From version 1.0.0.

Where `<variable>` is a known identifier and `<value>` is a number, a sequence of non-blank
chars, or a `"`-quoted string (allowing `\"` in the string to represent the quotation mark).

Variable names start with `[a-zA-Z]` followed by zero or more of `[a-zA-Z0-9_-]`.

Version 1.0.0 integer variables and their units and ranges:

* `enabled`, boolean, 0 or 1, if 0 then no observations are uploaded
* `mqtt-use-tls`, boolean, 0 or 1, if 1 then a root certificate must be defined
* `observation-interval`, seconds, 60 (one minute) to 28800 (eight hours), how often observations
   are recorded for upload.
* `upload-interval`, seconds, 60 (one minute) to 172800 (48 hours), how long to wait before trying
   to upload pending observations.  Note that there's a fixed-length observation queue on the device;
   if the observation frequency is high and the upload frequency is low then some observations will
   be discarded
* `mqtt-endpoint-port`, port number, 1000 to 65535 (TBD), the port number of the mqtt server

Version 1.0.0 string variables and their formats:

* `device-id`, string, the device ID, used by the back-end databases
* `device-class`, string, the class ID for the device, used by the back-end databases
* `ssid1`, `ssid2`, `ssid3`, string, the SSIDs for up to three WiFi networks
* `password`, `password2`, `password3`, string, the passwords for up to three WiFi networks
* `mqtt-endpoint-host`, string, the host name or IP address of the mqtt server
* `mqtt-id`, string, the ID the device used to identify itself to the mqtt server, defaults to the `device-id`
* `mqtt-root-cert`, string, the root certificate for TLS connections
* `mqtt-auth`, string, the authentication mechanism, either "pass" (for user+password) or "x509"
* `mqtt-device-cert`, string, the device certificate for x509 authentication
* `mqtt-private-key`, string, the device private key for x509 authentication
* `mqtt-username`, string, the username for user+password authentication
* `mqtt-password`, string, the password for user+password authentication

### Statement `set <variable> @<value>`

From version 1.0.0.

Where `<variable>` is a known identifier and `<value>` is the name of a file containing the actual
value.  The `<value>` in the statement *not* including the `@` can be quoted or not, as for `set`.

if the `<value>` after unquoting has the initial sequence `~/` then the `~` is expanded to the
user's home directory taken from `$HOME`.

Apart from the indirection, this form of `set` behaves like the other form of `set`.

(Rationale: Tilde expansion is useful to keep config source files, which can be checked into SCM,
apart from the secrets that the compiled file includes.)

TODO: Would it be useful to specify what relative paths mean, are they relative to the config
file or relative to the cwd?

### Statement `end`

From version 1.0.0.

Don't save configuration but end the script.  Must be last if it appears.

### Statement `save`

From version 1.0.0.

Save the configuration in nvram or similar.  If a `clear` directive was present this will wipe all
configuration options from nvram first.

Must be last if it appears.


## Output language

(The output language is not interesting to users.)

The output is a sequence of instructions.  It always starts with `START` and ends with `STOP`.

The output is such that it can be syntax checked before execution, and if the syntax check passes
and the version is supported then there will be no errors during execution.  On the other hand, this
guarantee is not currently useful because nothing is committed until `STOP`, and the `STOP` does not
fail.

```
    START <x> <y> <z> <w>   -- check version and clear
        where START = 1 : u8
          and <x>, <y>, <z> are u8 values (version bytes)
          and <w> is a u8 value (flag byte)
        Succeeds if the processor knows how to handle version x.y.z.
        Resets the config to factory defaults if (w & 1) != 0

    STOP <w>               -- stop processing and maybe save
         where STOP = 2 : u8
           and <w> is a u8 value (flag byte)
         Saves the config to nvram if (w & 1) != 0

    SETI <n> <m>          -- set int variable
        where SETI = 3 : u8
          and <n> is the known index of the variable in question, a u8
          and <m> is the value being set, an i32 in little-endian format

    SETS <n> <m> <c> ...   -- set string variable or certificate
        where SETS = 4 : u8
          and <n> is the known index of the variable in question, a u8
          and <m> is the number of utf8 bytes in the value being set, a u16 in little-endian format
          and <c> ... are the utf8 bytes in the value being set, all u8
```

List of known variable names and their types, with their indices (which are type-specific):

In Version 1.0.0:

```
    enabled     : int       0
    <LoRa variable> ...     1 .. 

    device-name : string    0
    <LoRa variable> ...     1 .. 
```
