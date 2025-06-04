#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/**
 * Reproduce un sonido a la frecuencia especificada
 */
void audio_play(uint16_t frequency);

/**
 * Delay
 */
void audio_delay(uint16_t ms);

/**
 * Detiene cualquier sonido que se esté reproduciendo
 */
void audio_stop(void);

/**
 * Apaga el sistema de sonido
 */
void audio_shutdown(void);

/**
 * Reproduce un beep
 */
void audio_beep(uint16_t frequency, uint16_t duration);

/**
 * Reproduce una secuencia de tonos
 */
void audio_tone_sequence(const uint16_t* frequencies, const uint16_t* durations, uint8_t count);

#endif /* AUDIO_H */

