#include <audio.h>
#include <stdint.h>
#include <time.h> // Falta implementarlo bien?

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


void audio_play(uint16_t frequency) {
  if (frequency <= 1) {
    audio_stop();
    return;
  }

  uint32_t divisor = PIT_OSCILLATOR_FREQ / frequency;

  /* Configura el canal 2 del PIT a la frecuencia deseada */
  /* PIT recibe de a 8 bits --> Byte bajo primero, alto despues por LE */
  outb(PIT_MODE_COMMAND_PORT, PIT_CHANNEL2_MODE);
  outb(PIT_CHANNEL2_DATA_PORT, (uint8_t) (divisor));      /* Byte bajo */
  outb(PIT_CHANNEL2_DATA_PORT, (uint8_t) (divisor >> 8)); /* Byte alto */

  /* Conecta el canal 2 del PIT al PC Speaker habilitando los bits 0-1 */
  uint8_t tmp = inb(PC_SPEAKER_PORT);
  if ((tmp & SPEAKER_ENABLE_BIT) != SPEAKER_ENABLE_BIT)
    outb(PC_SPEAKER_PORT, tmp | SPEAKER_ENABLE_BIT);
}

void audio_stop(void) {
  /* Desconecta el PC Speaker del canal 2 del PIT */
  uint8_t tmp = inb(PC_SPEAKER_PORT) & ~SPEAKER_ENABLE_BIT;
  outb(PC_SPEAKER_PORT, tmp);
}

// Con el time.c de la catedra
static void audio_delay(uint16_t ms) {
    uint16_t start = ticks_elapsed();
    uint16_t elapsed;

    do {
        elapsed = ticks_elapsed() - start;
    } while (elapsed < ms);
}

void audio_beep(uint16_t frequency, uint16_t duration) {
  if (frequency > 1) audio_play(frequency);
  audio_delay(duration);
  audio_stop();
}

void audio_shutdown(void) { audio_stop(); }

