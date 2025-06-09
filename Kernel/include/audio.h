#ifndef AUDIO_H
#define AUDIO_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint16_t frequency; // Frecuencia en Hz
  uint16_t duration;  // Duración en ms
} audio_note_t;

// Frecuencia inválida para marcar el fin de una melodía
#define AUDIO_END_MELODY 0xFFFF

/**
 * Reproduce un sonido a la frecuencia especificada
 */
void audio_play(uint16_t frequency);

/**
 * Detiene cualquier sonido que se esté reproduciendo
 */
void audio_stop(void);

/**
 * Reproduce un beep
 */
void audio_beep(uint16_t frequency, uint16_t duration);

/**
 * Reproduce una melodía definida por un arreglo de notas
 */
void audio_play_melody(const uint16_t* freqs, const uint16_t* durs, uint32_t count);

/**
 * Función para el timer_handler
 */
void audio_timer_tick(void);

#endif /* AUDIO_H */

