#include <syscall.h>
#include <sound.h>

#define TEMPO 120 // BPM

// Note frequency table (in Hz) - Scientific pitch notation
static const uint16_t NOTE_FREQUENCIES[] = {
    [REST] = 0,  [C2] = 65,    [CS2] = 69,  [D2] = 73,    [DS2] = 78,
    [E2] = 82,   [F2] = 87,    [FS2] = 92,  [G2] = 98,    [GS2] = 104,
    [A2] = 110,  [AS2] = 117,  [B2] = 123,

    [C3] = 131,  [CS3] = 139,  [D3] = 147,  [DS3] = 156,  [E3] = 165,
    [F3] = 175,  [FS3] = 185,  [G3] = 196,  [GS3] = 208,  [A3] = 220,
    [AS3] = 233, [B3] = 247,

    [C4] = 262,  [CS4] = 277,  [D4] = 294,  [DS4] = 311,  [E4] = 330,
    [F4] = 349,  [FS4] = 370,  [G4] = 392,  [GS4] = 415,  [A4] = 440,
    [AS4] = 466, [B4] = 494,

    [C5] = 523,  [CS5] = 554,  [D5] = 587,  [DS5] = 622,  [E5] = 659,
    [F5] = 698,  [FS5] = 740,  [G5] = 784,  [GS5] = 831,  [A5] = 880,
    [AS5] = 932, [B5] = 988,

    [C6] = 1047, [CS6] = 1109, [D6] = 1175, [DS6] = 1245, [E6] = 1319,
    [F6] = 1397, [FS6] = 1480, [G6] = 1568};

/**
 * Calculate duration in milliseconds based on note duration type
 */
static uint16_t calculate_duration(note_duration_t duration) {
  uint16_t base_duration = (60000 * 4) / TEMPO; // Whole note duration

  switch (duration) {
  case DURATION_WHOLE:
    return base_duration;
  case DURATION_HALF:
    return base_duration / 2;
  case DURATION_QUARTER:
    return base_duration / 4;
  case DURATION_EIGHTH:
    return base_duration / 8;
  case DURATION_SIXTEENTH:
    return base_duration / 16;
  default:
    return base_duration / 4;
  }
}

void sound_beep(uint16_t frequency, uint16_t duration_ms) {
  _syscall(SYS_AUDIO_BEEP, frequency, duration_ms);
}

/**
 * Play a single note with specified duration
 */
static void play_single_note(note_t note, uint16_t duration_ms) {
  uint16_t frequency = NOTE_FREQUENCIES[note];
  _syscall(SYS_AUDIO_BEEP, frequency, duration_ms);
}

void sound_play_note(note_t note, note_duration_t duration) {
  uint16_t duration_ms = calculate_duration(duration);
  play_single_note(note, duration_ms);
}

void sound_play_melody(const melody_note_t *melody, uint32_t length) {
  if (!melody || length == 0)
    return;

  for (uint32_t i = 0; i < length; i++) {
    sound_play_note(melody[i].note, melody[i].duration);

    // Small gap between notes for clarity
    if (i < length - 1 && melody[i].note != REST) {
      _syscall(SYS_AUDIO_BEEP, 0, 30);
    }
  }
}

/**
 * Play the Tetris theme melody
 */
void sound_play_tetris(void) {
  static const melody_note_t tetris[] = {
      {E5, DURATION_QUARTER}, {B4, DURATION_EIGHTH},
      {C5, DURATION_EIGHTH},  {D5, DURATION_QUARTER},
      {C5, DURATION_EIGHTH},  {B4, DURATION_EIGHTH},

      {A4, DURATION_QUARTER}, {A4, DURATION_EIGHTH},
      {C5, DURATION_EIGHTH},  {E5, DURATION_QUARTER},
      {D5, DURATION_EIGHTH},  {C5, DURATION_EIGHTH},

      {B4, DURATION_QUARTER}, {B4, DURATION_EIGHTH},
      {C5, DURATION_EIGHTH},  {D5, DURATION_QUARTER},
      {E5, DURATION_QUARTER},

      {C5, DURATION_QUARTER}, {A4, DURATION_QUARTER},
      {A4, DURATION_HALF},    {REST, DURATION_EIGHTH},

      {D5, DURATION_QUARTER}, {F5, DURATION_EIGHTH},
      {A5, DURATION_QUARTER}, {G5, DURATION_EIGHTH},
      {F5, DURATION_EIGHTH},

      {E5, DURATION_QUARTER}, {E5, DURATION_EIGHTH},
      {C5, DURATION_EIGHTH},  {E5, DURATION_QUARTER},
      {D5, DURATION_EIGHTH},  {C5, DURATION_EIGHTH},

      {B4, DURATION_QUARTER}, {B4, DURATION_EIGHTH},
      {C5, DURATION_EIGHTH},  {D5, DURATION_QUARTER},
      {E5, DURATION_QUARTER},

      {C5, DURATION_QUARTER}, {A4, DURATION_QUARTER},
      {A4, DURATION_HALF}};

  sound_play_melody(tetris, sizeof(tetris) / sizeof(tetris[0]));
}

/*
 * Basic and util sounds
 */
void sound_shell_beep(void) { sound_beep(1000, 300); }

void sound_ball_hit(void) { sound_beep(1200, 150); }

