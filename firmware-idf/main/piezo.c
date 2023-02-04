#include "piezo.h"

#ifdef SNAPPY_GPIO_PIEZO

// When CMD_START is sent, we must also send a char* with the music
enum class PlayerOp {
    STOP,
    START,
};

struct PlayerCommand {
  enum Op { STOP, START };
  Op op;
  const music_t melody;  // For START
};

// SYNCHRONIZATION
static QueueHandle_t command_queue;

// VARIABLES OWNED BY THE PLAYER TASK
static bool is_playing = false;
static uint8_t default_dur;
static uint8_t default_oct;
static uint8_t bpm;
static uint32_t wholenote;
static const struct tone *next_tone;
static const struct tone *tone_limit;

static void start_playing(const struct music* melody) {
  next_tone = melody->tones;
  tone_limit = next_tone + melody->length;
}

static bool get_note(int* frequency, int* duration_ms) {
  if (next_tone == NULL || next_tone == tone_limit) {
    return false;
  }
  *frequency = next_tone->frequency;
  *duration_ms = next_tone->duration_ms;
  return true;
}

static void vPlayerTask(void* arg) {
  int prev_frequency = -1;
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
          prev_frequency = -1;
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
        if (prev_frequency == frequency) {
          // A bit of a hack.  Avoid running the notes together if they are the same.  This works
          // well for some tunes and less well for some others, and the delay probably ought to
          // be relative to the bpm of the song.
          stop_note();
          delay(10);
        }
        start_note(frequency);
        prev_frequency = frequency;
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

/* FIXME: This may have a different form here, the "main thread" may no longer exist?
   What if other tasks need to play a song? */

// VARIABLES OWNED BY THE SNAPPYSENSE MAIN THREAD
static bool is_initialized = false;

static void initialize() {
  setup_sound();
  command_queue = xQueueCreate(5, sizeof(PlayerCommand));
  /* FIXME: Priority is probably defined elsewhere */
  if (xTaskCreate( vPlayerTask, "player", 1024, nullptr, tskIDLE_PRIORITY+1, nullptr) != pdPASS) {
    panic("Could not create task");
  }
}

void play_song(const struct music* melody) {
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

#else /* SNAPPY_GPIO_PIEZO */
void play_song(const struct music* melody) {}
void stop_song() {}
#endif /* SNAPPY_GPIO_PIEZO */
