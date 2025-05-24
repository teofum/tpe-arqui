#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/**
 * Reproduce un sonido a la frecuencia especificada
 */
void audio_play(uint16_t frequency);

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

#endif /* AUDIO_H */

