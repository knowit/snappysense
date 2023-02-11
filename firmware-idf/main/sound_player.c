/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "sound_player.h"

#ifdef SNAPPY_SOUND_EFFECTS

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "device.h"

typedef enum {
    STOP,
    START,
} PlayerOp;

typedef struct {
  PlayerOp op;
  const struct music* melody;  /* For START command */
} PlayerCommand;

/* SYNCHRONIZATION */
static QueueHandle_t command_queue;

/* VARIABLES OWNED BY THE PLAYER TASK */
static bool is_playing = false;
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
    /* If a command is pending, it takes precedence.  We're between notes now, so it's OK both to
       stop the song and to start a new one, and to. */
    if (xQueueReceive(command_queue, &cmd, 0)) {
      switch (cmd.op) {
        case STOP:
          if (is_playing) {
            stop_note();
            is_playing = false;
          }
          break;
        case START:
          if (is_playing) {
            stop_note();
            is_playing = false;
          }
          start_playing(cmd.melody);
          prev_frequency = -1;
          is_playing = true;
          break;
        default:
          LOG("Bad player op %d", cmd.op);
          break;
      }
      continue;
    }
    /* If we're playing, get a note to play and play it, and then wait until it has finished
       playing before doing something else. */
    if (is_playing) {
      int frequency, duration_ms;
      if (get_note(&frequency, &duration_ms)) {
        if (prev_frequency == frequency) {
          /* A bit of a hack.  Avoid running the notes together if they are the same.  This works
             well for some tunes and less well for some others, and the delay probably ought to be
             relative to the bpm of the song.  It's possible the music compiler should instead embed
             this in the song directly. */
          stop_note();
          vTaskDelay(pdMS_TO_TICKS(10));
        }
        start_note(frequency);
        prev_frequency = frequency;
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
      } else {
        stop_note();
        is_playing = false;
      }
      continue;
    }
    /* Not playing, and no commands pending, so wait for some to arrive. */
    xQueuePeek(command_queue, &cmd, portMAX_DELAY);
  }
}

bool sound_effects_begin() {
  command_queue = xQueueCreate(5, sizeof(PlayerCommand));
  return xTaskCreate( vPlayerTask, "player", 1024, NULL, PLAYER_PRIORITY, NULL) == pdPASS;
}

void sound_effects_play(const struct music* melody) {
  PlayerCommand cmd = { START, melody };
  xQueueSend(command_queue, &cmd, portMAX_DELAY);
}

void sound_effects_stop() {
  PlayerCommand cmd = { STOP, NULL };
  xQueueSend(command_queue, &cmd, portMAX_DELAY);
}

#endif /* SNAPPY_SOUND_EFFECTS */
