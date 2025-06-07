#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

// ============================================================================
// TYPES AND ENUMS
// ============================================================================

/**
 * Musical notes
 */
typedef enum {
    REST = 0,

    // Octave 2
    C2, CS2, D2, DS2, E2, F2, FS2, G2, GS2, A2, AS2, B2,

    // Octave 3
    C3, CS3, D3, DS3, E3, F3, FS3, G3, GS3, A3, AS3, B3,

    // Octave 4 (Middle octave)
    C4, CS4, D4, DS4, E4, F4, FS4, G4, GS4, A4, AS4, B4,

    // Octave 5
    C5, CS5, D5, DS5, E5, F5, FS5, G5, GS5, A5, AS5, B5,

    // Octave 6
    C6, CS6, D6, DS6, E6, F6, FS6, G6,

    NOTE_COUNT  // Total number of notes
} note_t;

/**
 * Note duration types
 */
typedef enum {
    DURATION_WHOLE = 0,
    DURATION_HALF,
    DURATION_QUARTER,
    DURATION_EIGHTH,
    DURATION_SIXTEENTH
} note_duration_t;

/**
 * Struct for melody notes
 */
typedef struct {
    note_t note;                // The musical note
    note_duration_t duration;   // Duration (quarter, half, etc.)
} melody_note_t;


// ============================================================================
// FUNCTIONS PROTOTYPES
// ============================================================================

void sound_beep(uint16_t frequency, uint16_t duration_ms);

void sound_play_note(note_t note, note_duration_t duration);

void sound_play_melody(const melody_note_t* melody, uint32_t length);

void sound_play_tetris(void);

void sound_stop(void);

void sound_shell_beep(void);

void sound_ball_hit(void);

#endif // SOUND_H

