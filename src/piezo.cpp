#include "piezo.h"

#ifdef SNAPPY_PIEZO

#include "device.h"
#include "util.h"

#define RTTTL_OCTAVE_OFFSET 0 // offset octave, as needed

typedef enum {
  PAUSE = 0,
  NOTE_B0 = 33,
  NOTE_CS1 = 35,
  NOTE_D1 = 37,
  NOTE_DS1 = 39,
  NOTE_E1 = 41,
  NOTE_F1 = 44,
  NOTE_FS1 = 46,
  NOTE_G1 = 49,
  NOTE_GS1 = 52,
  NOTE_A1 = 55,
  NOTE_AS1 = 58,
  NOTE_B1 = 62,
  NOTE_C2 = 65,
  NOTE_CS2 = 69,
  NOTE_D2 = 73,
  NOTE_DS2 = 78,
  NOTE_E2 = 82,
  NOTE_F2 = 87,
  NOTE_FS2 = 93,
  NOTE_G2 = 98,
  NOTE_GS2 = 104,
  NOTE_A2 = 110,
  NOTE_AS2 = 117,
  NOTE_B2 = 123,
  NOTE_C3 = 131,
  NOTE_CS3 = 139,
  NOTE_D3 = 147,
  NOTE_DS3 = 156,
  NOTE_E3 = 165,
  NOTE_F3 = 175,
  NOTE_FS3 = 185,
  NOTE_G3 = 196,
  NOTE_GS3 = 208,
  NOTE_A3 = 220,
  NOTE_AS3 = 233,
  NOTE_B3 = 247,
  NOTE_C4 = 262,
  NOTE_CS4 = 277,
  NOTE_D4 = 294,
  NOTE_DS4 = 311,
  NOTE_E4 = 330,
  NOTE_F4 = 349,
  NOTE_FS4 = 370,
  NOTE_G4 = 392,
  NOTE_GS4 = 415,
  NOTE_A4 = 440,
  NOTE_AS4 = 466,
  NOTE_B4 = 494,
  NOTE_C5 = 523,
  NOTE_CS5 = 554,
  NOTE_D5 = 587,
  NOTE_DS5 = 622,
  NOTE_E5 = 659,
  NOTE_F5 = 698,
  NOTE_FS5 = 740,
  NOTE_G5 = 784,
  NOTE_GS5 = 831,
  NOTE_A5 = 880,
  NOTE_AS5 = 932,
  NOTE_B5 = 988,
  NOTE_C6 = 1047,
  NOTE_CS6 = 1109,
  NOTE_D6 = 1175,
  NOTE_DS6 = 1245,
  NOTE_E6 = 1319,
  NOTE_F6 = 1397,
  NOTE_FS6 = 1480,
  NOTE_G6 = 1568,
  NOTE_GS6 = 1661,
  NOTE_A6 = 1760,
  NOTE_AS6 = 1865,
  NOTE_B6 = 1976,
  NOTE_C7 = 2093,
  NOTE_CS7 = 2217,
  NOTE_D7 = 2349,
  NOTE_DS7 = 2489,
  NOTE_E7 = 2637,
  NOTE_F7 = 2794,
  NOTE_FS7 = 2960,
  NOTE_G7 = 3136,
  NOTE_GS7 = 3322,
  NOTE_A7 = 3520,
  NOTE_AS7 = 3729,
  NOTE_B7 = 3951,
  NOTE_C8 = 4186,
  NOTE_CS8 = 4435,
  NOTE_D8 = 4699,
  NOTE_DS8 = 4978
} NOTES;

NOTES notes[] = { PAUSE, NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4,
    NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4, NOTE_C5, NOTE_CS5,
    NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5,
    NOTE_AS5, NOTE_B5, NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6,
    NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6, NOTE_C7, NOTE_CS7,
    NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7,
    NOTE_AS7, NOTE_B7 };

// When CMD_START is sent, we must also send a char* with the music
enum class PlayerOp {
    STOP,
    START,
};

struct PlayerCommand {
  enum Op { STOP, START };
  Op op;
  const char* melody;  // For START
};

// SYNCHRONIZATION
static QueueHandle_t command_queue;

// VARIABLES OWNED BY THE PLAYER TASK
static bool is_playing = false;
static uint8_t default_dur;
static uint8_t default_oct;
static uint8_t bpm;
static uint32_t wholenote;
static const char *tune;

static void start_playing(const char* melody) {
  int num;

  tune = melody;

  // load up some safe defaults
  default_dur = 4;
  default_oct = 6;
  bpm = 63;

  // Syntax:
  //  Melody ::= Preamble Defaults Notes?
  //  Preamble ::= /[^:]*:/
  //  Defaults ::= /d=(\d*),o=(\d),b=(\d*):/
  //     where \1 is the duration in fractions of a whole note (default 4)
  //           \2 is the octave (default 6 but this is immaterial)
  //           \3 is the beats per minute (default 63)
  //  Notes ::= Note ("," Note)*
  //  Note ::= /(\d+)?([cdefgabp])(#)?(\.)?(\d)?/
  //     where \1 is the duration in fractions of a whole note (default 4)
  //           \2 is the note
  //           \3 is the optional # mark
  //           \4 is the optional duration lengthening mark
  //           \5 is the optional octave (default 6)

  // Skip the preamble
  while (*tune && *tune != ':') {
    tune++; // ignore name
  }
  if (!*tune) { goto borked; }
  tune++; // skip ':'

  // Parse the defaults
  // get duration
  if (tune[0] != 'd' || tune[1] != '=') { goto borked; }
  tune += 2;
  num = 0;
  while (isdigit(*tune)) {
    num = (num * 10) + (*tune++ - '0');
  }
  if (num > 0) {
    default_dur = num;
  }
  if (*tune != ',') { goto borked; }
  tune++;

  // get octave
  if (tune[0] != 'o' || tune[1] != '=') { goto borked; }
  tune += 2;
  if (!isdigit(*tune)) { goto borked; }
  num = *tune++ - '0';
  if (num >= 3 && num <= 7) {
    default_oct = num;
  }
  if (*tune != ',') { goto borked; }
  tune++;

  // get BPM
  if (tune[0] != 'b' || tune[1] != '=') { goto borked; }
  tune += 2;
  num = 0;
  while (isdigit(*tune)) {
    num = (num * 10) + (*tune++ - '0');
  }
  if (bpm != 0) {
    bpm = num;
  }
  if (*tune != ':') { goto borked; }
  tune++;

  // BPM usually expresses the number of quarter notes per minute
  // calculate time for whole note (in milliseconds)
  wholenote = (60 * 1000L / bpm) << 2;
  return;

borked:
  log("Bad tune\n");
}

// Get the next note from input string and return true if there's one, otherwise false.
// We return the physical frequency in *frequency and the duration in ms in *duration
static bool get_note(int* frequency, int* duration_ms) {
  int note, scale;
  int num = 0;

  if (!*tune) {
    return false;
  }
  // calculate note duration
  while (isdigit(*tune))
    num = (num * 10) + (*tune++ - '0');

  *duration_ms = wholenote / (num ? num : default_dur);

  // get next note
  switch (*tune++) {
  case 'c':
    note = 1;
    break;
  case 'd':
    note = 3;
    break;
  case 'e':
    note = 5;
    break;
  case 'f':
    note = 6;
    break;
  case 'g':
    note = 8;
    break;
  case 'a':
    note = 10;
    break;
  case 'b':
    note = 12;
    break;
  case 'p':
    note = 14;
    break;
  default:
    note = 0;
  }

  // check for optional '#' sharp
  if (*tune == '#') {
    note++;
    tune++;
  }
  // check for optional '.' dotted note
  if (*tune == '.') {
    *duration_ms += (*duration_ms >> 1);
    tune++;
  }
  // get scale
  if (isdigit(*tune)) {
    scale = *tune++ - '0';
  } else {
    scale = default_oct;
  }
  scale += RTTTL_OCTAVE_OFFSET;

  if (*tune == ',') {
    tune++; // skip comma for next note
  }
  *frequency = note ? notes[(scale - 4) * 12 + note] : 440;
  return true;
}

static void vPlayerTask(void* arg) {
  for(;;) {
    PlayerCommand cmd;
    // If a command is pending, it takes precedence.  We're between notes now, so it's OK
    // both to stop the song and to start a new one, and to
    if (xQueueReceive(command_queue, &cmd, 0)) {
      switch (cmd.op) {
        case PlayerCommand::STOP:
          if (is_playing) {
            stop_note();
            is_playing = false;
          }
          break;
        case PlayerCommand::START:
          if (is_playing) {
            stop_note();
            is_playing = false;
          }
          start_playing(cmd.melody);
          is_playing = true;
          break;
        default:
          log("Bad player op %d", cmd.op);
          break;
      }
      continue;
    }
    // If we're playing, get a note to play and play it, and then wait until it has finished
    // playing before doing something else.
    if (is_playing) {
      int frequency, duration_ms;
      if (get_note(&frequency, &duration_ms)) {
        start_note(frequency);
        delay(duration_ms);
      } else {
        stop_note();
        is_playing = false;
      }
      continue;
    }
    // Not playing, and no commands pending, so wait for some to arrive.
    xQueuePeek(command_queue, &cmd, portMAX_DELAY);
  }
}

// VARIABLES OWNED BY THE SNAPPYSENSE MAIN THREAD
static bool is_initialized = false;

static void initialize() {
  setup_sound();
  command_queue = xQueueCreate(5, sizeof(PlayerCommand));
  if (xTaskCreate( vPlayerTask, "player", 1024, nullptr, tskIDLE_PRIORITY+1, nullptr) != pdPASS) {
    panic("Could not create task");
  }
}

void play_song(const char* melody) {
    if (!is_initialized) {
        initialize();
        is_initialized = true;
    }
    PlayerCommand cmd { PlayerCommand::START, melody };
    xQueueSend(command_queue, &cmd, portMAX_DELAY);
}

void stop_song() {
    PlayerCommand cmd { PlayerCommand::STOP, nullptr };
    xQueueSend(command_queue, &cmd, portMAX_DELAY);
}

#endif // SNAPPY_PIEZO
