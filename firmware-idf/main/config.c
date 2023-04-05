/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* This is a reader/interpreter for the binary config file format defined by the configuration
   compiler, see util/config-compiler/README.md at the SnappySense root */

#error "This code is not finished"

/* BEGIN constants that are fixed by the spec */

/* Instructions */
#define INSTR_START 1		/* START v0 v1 v2 flag */
#define INSTR_STOP 2		/* STOP flag */
#define INSTR_SETI 3		/* SETI var ii ii ii ii */
#define INSTR_SETS 4		/* SETS var uu uu byte ... */

/* Integer variables */
#define IVAR_ENABLED 0
#define IVAR_OBSERVATION_INTERVAL 1
#define IVAR_UPLOAD_INTERVAL 2
#define IVAR_MQTT_ENDPOINT_PORT 3
#define IVAR_MQTT_USE_TLS 4
#define NUM_IVARS 5		/* Version 1.0.0 */

/* String (and certificate) variables */
#define SVAR_DEVICE_ID 0
#define SVAR_DEVICE_CLASS 1
#define SVAR_SSID1 2
#define SVAR_SSID2 3
#define SVAR_SSID3 4
#define SVAR_PASSWORD1 5
#define SVAR_PASSWORD2 6
#define SVAR_PASSWORD3 7
#define SVAR_MQTT_ENDPOINT_HOST 8
#define SVAR_MQTT_ID 9
#define SVAR_MQTT_ROOT_CERT 10
#define SVAR_MQTT_AUTH 11
#define SVAR_MQTT_DEVICE_CERT 12
#define SVAR_MQTT_PRIVATE_KEY 13
#define SVAR_MQTT_USERNAME 14
#define SVAR_MQTT_PASSWORD 15
#define NUM_SVARS 16		/* Version 1.0.0 */

/* END constants that are fixed by the spec */

/* Entries in ivars are signed 32-bit integers; zero if unset/default (for now).  In nvram the keys for
   these are i<nn> where <nn> is the decimal index. */
static int32_t ivars[NUM_IVARS];

/* Entries in svars[] point to malloc'd storage representing NUL-terminated UTF8 strings; NULL if
   unset/default (for now).  In nvram the keys for these are s<nn> where <nn> is the decimal
   index. */
static char* svars[NUM_SVARS];

static bool run_configuration(const uint8_t* config, const uint8_t* limit, bool perform);

/* Read configuration from the instruction stream into internal tables, note this may also save the
   config to nvram.  We first check that the config is consistent, and then we execute it. */
bool accept_configuration(const uint8_t* config, size_t len) {
  return run_configuration(config, config+len, false) && 
         run_configuration(config, config+len, true);
}

/* Clear the in-RAM configuration to defaults. */
static void clear_configuration() {
  int i;
  for ( i=0 ; i < NUM_IVARS; i++ ) {
    ivars[i] = 0;
  }
  for ( i=0 ; i < NUM_SVARS ; i++ ) {
    free(svars[i]);
    svars[i] = NULL;
  }
}

/* Read configuration from nvram into internal tables, returns true if there was no failure in
   accessing any field other than the field not existing. */
bool read_configuration() {
}

/* Save settings.  We only save settings that have values different from default (zero for integers
   and NULL for strings). */
static void save_configuration() {
}

/* Clear all config settings in nvram, to avoid lingering garbage from affecting us. */
static void nuke_nvram() {
}

static bool run_configuration(const uint8_t* config, const uint8_t* limit, bool perform) {
  bool clear = false;
  bool save = false;

  /* START v0 v1 v2 flag */
  if ((limit - config) < 5 || config[0] != INSTR_START) {
    return false;
  }
  if (config[1] != 1 || config[2] != 0 || config[3] != 0) {
    return false;
  }
  if (config[4] != 0 && config[4] != 1) {
    return false;
  }
  clear = config[4] & 1;
  config += 5;

  /* Evaluate START */
  if (perform && clear) {
    clear_configuration();
  }

  /* Body */
  while (config < limit) {
    switch (*limit) {
    case INSTR_STOP:
      break;

    case INSTR_SETI: {
      /* SETI var nn nn nn nn */
      if ((limit - config) < 6) {
	return false;
      }
      unsigned var = config[1];
      int32_t val = int32_t(config[2]) | (int32_t(config[3]) << 8) | (int32_t(config[4]) << 16) | (int32_t(config[6]) << 24);
      if (var >= NUM_IVARS) {
	return false;
      }
      /* TODO: Presumably some variables have specific ranges; we could check them here.  Notably,
	 some could be required to be nonnegative.  Or we could have a SETU instruction for that. */
      config += 6;

      /* Evaluate SETI */
      if (perform) {
	ivars[var] = val;
      }
      break;
    }

    case INSTR_SETS: {
      /* SETS var nn nn byte ... */
      if ((limit - config) < 4) {
	return false;
      }
      unsigned var = config[1];
      unsigned len = unsigned(config[2]) | (unsigned(config[3]) << 8);
      config += 4;

      if ((limit - config) < len) {
	return false;
      }
      const uint8_t* val = config + 4;
      /* TODO: Check that there's no NUL in the data, at least */
      config += len;

      /* Evaluate SETS */
      if (perform) {
	char* mem = malloc(len+1);
	memcpy(mem, val, len);
	mem[len] = 0;
	free(svars[var]);
	svars[var] = mem;
      }
      break;
    }

    default:
      return false;
    }
  }

  /* STOP flag */
  if ((limit - config) < 2 || config[0] != INSTR_STOP) {
    return false;
  }
  if (config[1] != 0 && config[1] != 1) {
    return false;
  }
  save = config[1] & 1;
  config += 2;

  /* Evaluate STOP */
  if (perform && save) {
    if (clear) {
      nuke_nvram();
    }
    save_configuration();
  }

  /* End of input should be right here. */
  if (limit != config) {
    return false;
  }

  return true;
}
