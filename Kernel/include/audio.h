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
 * Reproduce un beep
 */
void audio_beep(uint16_t frequency, uint16_t duration);

/**
 * Returns 1 if audio running, 0 if not
 */
uint8_t audio_status(void);

#endif /* AUDIO_H */

