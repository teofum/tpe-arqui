#include <audio.h>
#include <libasm.h>
#include <stdint.h>
#include <time.h>

/* Funciones externas de audio_asm */
/**
 * Lee un byte de un puerto de E/S y devuelve valor leído del puerto
 */
extern uint8_t inb(uint16_t port);

/**
 * Escribe un byte en un puerto de E/S
 */
extern void outb(uint16_t port, uint8_t value);

/* Constantes relacionadas con el PC Speaker */
#define PIT_CHANNEL2_DATA_PORT                                                 \
  0x42                             /* Puerto de datos del canal 2 para el PIT */
#define PIT_MODE_COMMAND_PORT 0x43 /* Puerto de comandos de modo del PIT */
#define PC_SPEAKER_PORT 0x61       /* Puerto de control del PC Speaker */

/* Constantes relacionadas con el PIT */
#define PIT_OSCILLATOR_FREQ 1193180 /* Frecuencia base para el PIT en Hz */
#define PIT_CHANNEL2_MODE                                                      \
  0xB6 /* Modo para el canal 2: generador de onda cuadrada */

/* Bits de control del Speaker */
#define SPEAKER_ENABLE_BIT 0x03 /* Bits para habilitar el PC speaker*/
#define SPEAKER_DISABLE_BIT 0xFC

/* Estado del driver de audio */
static struct {
  audio_note_t *current_melody; // Array interno de notas
  uint32_t current_note_index;  // Índice de la nota actual
  uint32_t countdown;           // Countdown para la nota actual
  uint8_t is_playing_melody;    // Flag
  uint32_t melody_length;       // Longitud del array de notas
} audio_state = {0};

// Array para Beep simple
static audio_note_t single_beep[2] = {
    {0, 0},               // Nota a reproducir
    {AUDIO_END_MELODY, 0} // Marcador de fin
};

#define MAX_NOTES 512

// Array para melodías
static audio_note_t melody_buffer[MAX_NOTES];

void audio_play(uint16_t frequency) {
  if (frequency == 0) {
    audio_stop();
    return;
  }

  uint32_t divisor = PIT_OSCILLATOR_FREQ / frequency;

  /* Configura el canal 2 del PIT a la frecuencia deseada */
  /* PIT recibe de a 8 bits --> Byte bajo primero, alto despues por LE */
  outb(PIT_MODE_COMMAND_PORT, PIT_CHANNEL2_MODE);
  outb(PIT_CHANNEL2_DATA_PORT, (uint8_t)(divisor));      /* Byte bajo */
  outb(PIT_CHANNEL2_DATA_PORT, (uint8_t)(divisor >> 8)); /* Byte alto */

  /* Conecta el canal 2 del PIT al PC Speaker habilitando los bits 0-1 */
  uint8_t tmp = inb(PC_SPEAKER_PORT);
  if ((tmp & SPEAKER_ENABLE_BIT) != SPEAKER_ENABLE_BIT)
    outb(PC_SPEAKER_PORT, tmp | SPEAKER_ENABLE_BIT);
}

void audio_stop(void) {
  /* Desconecta el PC Speaker del canal 2 del PIT */
  uint8_t tmp = inb(PC_SPEAKER_PORT) & SPEAKER_DISABLE_BIT;
  outb(PC_SPEAKER_PORT, tmp);
}

void audio_beep(uint16_t frequency, uint16_t duration) {
  _sti();

  single_beep[0].frequency = frequency;
  single_beep[0].duration = duration;

  audio_state.current_melody = single_beep;
  audio_state.current_note_index = 0;
  audio_state.is_playing_melody = 1;
  audio_state.melody_length = 2;

  if (frequency != 0) {
    audio_play(frequency);
    audio_state.countdown = (duration * TICKS_PER_SECOND) / 1000;
  }
}

void audio_play_melody(const uint16_t *freqs, const uint16_t *durs,
                       uint32_t count) {
  if (!freqs || !durs || count == 0 || count > 511)
    return;

  _sti();

  for (uint32_t i = 0; i < count; i++) {
    melody_buffer[i].frequency = freqs[i];
    melody_buffer[i].duration = durs[i];
  }

  melody_buffer[count].frequency = AUDIO_END_MELODY;
  melody_buffer[count].duration = 0;

  audio_state.current_melody = melody_buffer;
  audio_state.current_note_index = 0;
  audio_state.is_playing_melody = 1;
  audio_state.melody_length = count + 1;

  const audio_note_t *first_note = &melody_buffer[0];
  if (first_note->frequency != AUDIO_END_MELODY) {
    if (first_note->frequency != 0) {
      audio_play(first_note->frequency);
    } else {
      audio_stop();
    }
    audio_state.countdown = (first_note->duration * TICKS_PER_SECOND) / 1000;
  }
}

void audio_timer_tick(void) {
  if (!audio_state.is_playing_melody || !audio_state.current_melody) {
    return;
  }

  if (audio_state.countdown > 0) {
    audio_state.countdown--;
    return;
  }

  // La nota actual terminó, avanza a la siguiente
  audio_state.current_note_index++;

  // Por las dudas
  if (audio_state.current_note_index >= audio_state.melody_length) {
    audio_stop();
    return;
  }

  const audio_note_t *next_note =
      &audio_state.current_melody[audio_state.current_note_index];

  if (next_note->frequency == AUDIO_END_MELODY) {
    audio_stop(); // Termina la reproducción
    // Resetea el estado
    audio_state.current_melody = NULL;
    audio_state.current_note_index = 0;
    audio_state.countdown = 0;
    audio_state.is_playing_melody = 0;
    audio_state.melody_length = 0;
    return;
  }

  // Reproduce la siguiente nota
  // El caso de frecuencia 0 (silencio) se encarga audio_play
  audio_play(next_note->frequency);

  // Configura el countdown para la nueva nota
  audio_state.countdown = (next_note->duration * TICKS_PER_SECOND) / 1000;
}

