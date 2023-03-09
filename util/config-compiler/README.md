-*- fill-column: 100 -*-

# Configuration file compiler

This compiles a textual factory configuration file into a binary format.  The firmware interprets
only the binary format; the purpose of the indirection is to simplify the firmware.

There's a sample configuration file in `test.cfg`.

## Input language

Lines consisting entirely of whitespace are allowed.

Comment lines start with `#` following any leading whitespace.  Trailing comments are not allowed 
on the same line as a statement.

Otherwise the langage is a sequence of statements starting with `version` and ending with `end`
or `save`.

### Statement `version x.y.z`

From version 1.0.0.

The `x`, `y`, `z` are integers and the statement succeeds if the version stated can be understood by
the program reading the config (ie both the config compiler and the prefs parser on the device).
This statement must be first, and it must appear.

### Statement `clear`

From version 1.0.0.

Clear any saved configuration.  If this appears it must be second, following the `version`.

### Statement `set <variable> <value>`

From version 1.0.0.

Where `<variable>` is a known identifier and `<value>` is a number, a sequence of non-blank
chars, or a `"`-quoted string (allowing `\"` in the string to represent the quotation mark).

Version 1.0.0 variable names and their types are:

* `enabled`, integer
* `device-name`, string
* (tbd)

### Statement `cert <cert-name> @<filename>`

From version 1.0.0.

Where `<cert-name>` is a known identifier and `<filename>` references a file containing a
certificate (the whole value including the `@` can be quoted or not, as for string values).  If the
`<filename>` has the initial sequence `~/` then the `~` is expanded to the user's home directory.

Version 1.0.0 certificate names and their meanings are:

* (tbd)

### Statement `end`

From version 1.0.0.

Don't save configuration but end the script.  Must be last if it appears.

### Statement `save`

From version 1.0.0.

Save the configuration in nvram or similar.  Must be last if it appears.


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
