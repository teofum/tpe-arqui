#ifndef IDT_LOADER_H
#define IDT_LOADER_H

#include <stdint.h>

#define ID_TIMER_TICK 0x20
//#define ID_KEYBOARD 0x21
#define ID_SYSCALL 0x80

/* Descriptor de interrupcion */
typedef struct {
  uint16_t offset_l, selector;
  uint8_t zero, access;
  uint16_t offset_m;
  uint32_t offset_h, other_zero;
} idtDescriptor_t;

extern void picMasterMask(uint8_t mask);
extern void picSlaveMask(uint8_t mask);

static void setup_IDT_entry(int index, uint64_t offset);
void load_idt(void);

#endif

