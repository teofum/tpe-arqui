#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include <libasm.h>

#define ID_TIMER_TICK 0x20
//#define ID_KEYBOARD 0x21
#define ID_SYSCALL 0x80

#define MAX_INTERRUPTS 256
#define MAX_SYSCALLS 256

/* Descriptor de interrupción */
typedef struct {
  uint16_t offset_l, selector;
  uint8_t zero, access;
  uint16_t offset_m;
  uint32_t offset_h, other_zero;
} idtDescriptor_t;

void load_idt(void);

/* Handlers de interrupciones */
void _irq00Handler(void);
void _irq80Handler(void);
// void _irq01Handler(void);
// void _exception0Handler(void);

/* Manejo de IRQs */
void irqDispatcher(uint64_t irq);
void setInterruptHandler(uint64_t irq, void (*handler)());

/* Manejo de syscalls */
void initSyscalls(void);
void registerSyscall(uint64_t id, void *syscall);

/* Funciones externas asm */
extern void picMasterMask(uint8_t mask);
extern void picSlaveMask(uint8_t mask);
extern void _cli(void);
extern void _sti(void);

#endif /* INTERRUPTS_H */

