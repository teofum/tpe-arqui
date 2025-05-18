#include <idtLoader.h>
#include <defs.h>
#include <irqDispatcher.h>
#include <syscallDispatcher.h>
#include <interrupts.h>

extern void _irq00Handler();
extern void _irq01Handler();
extern void _irq80Handler();

#pragma pack(push)		/* Push de la alineación actual */
#pragma pack (1) 		/* Alinear las siguiente estructuras a 1 byte */

idtDescriptor_t * idt = (idtDescriptor_t *) 0;

static void setup_IDT_Entry (int index, uint64_t offset) {
  idt[index].selector = 0x08;
  idt[index].offset_l = offset & 0xFFFF;
  idt[index].offset_m = (offset >> 16) & 0xFFFF;
  idt[index].offset_h = (offset >> 32) & 0xFFFFFFFF;
  idt[index].access = ACS_INT;
  idt[index].zero = 0;
  idt[index].other_zero = (uint64_t) 0;
}

#pragma pack(pop)		/* Reestablece la alinceación actual */

void load_idt() {
  _cli();

  // IRQ 0: Timer tick
  setup_IDT_Entry(ID_TIMER_TICK, &_irq00Handler);

  // IRQ 1: Teclado
  // setupIdtEntry(ID_KEYBOARD, &_irq01Handler);

  // Syscalls
  setup_IDT_Entry(ID_SYSCALL, &_irq80Handler);

  picMasterMask(0xFC);
  picSlaveMask(0xFF);

  _sti();
}

