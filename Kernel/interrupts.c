#include <defs.h>
#include <interrupts.h>
#include <kbd.h>

idtDescriptor_t *idt = (idtDescriptor_t *) 0;
void (*irqHandlers[MAX_INTERRUPTS])();
void *syscallDispatchTable[MAX_SYSCALLS];

#pragma pack(push) /* Push de la alineación actual */
#pragma pack(1)    /* Alinear las siguiente estructuras a 1 byte */

/* Configura una entrada en la IDT */
static void setup_IDT_Entry(int index, uint64_t offset) {
  idt[index].selector = 0x08;
  idt[index].offset_l = offset & 0xFFFF;
  idt[index].offset_m = (offset >> 16) & 0xFFFF;
  idt[index].offset_h = (offset >> 32) & 0xFFFFFFFF;
  idt[index].access = ACS_INT;
  idt[index].zero = 0;
  idt[index].other_zero = (uint64_t) 0;
}

#pragma pack(pop) /* Reestablece la alineación actual */

/* Carga la IDT con las interrupciones configuradas */
void load_idt() {
  _cli();

  // IRQ 0: Timer tick
  setup_IDT_Entry(ID_TIMER_TICK, (uint64_t) &_irq00Handler);

  // IRQ 1: Teclado (comentado por ahora)
  // setup_IDT_Entry(ID_KEYBOARD, (uint64_t) &_irq01Handler);

  // Syscalls
  setup_IDT_Entry(ID_SYSCALL, (uint64_t) &_irq80Handler);

  picMasterMask(0xFE);
  picSlaveMask(0xFF);

  _sti();
}

/* Dispatcher de IRQs */
void irqDispatcher(uint64_t irq) {
  if (irq < MAX_INTERRUPTS) irqHandlers[irq]();
}

/* Registra un handler para una IRQ */
void setInterruptHandler(uint64_t irq, void (*handler)()) {
  if (irq < MAX_INTERRUPTS) irqHandlers[irq] = handler;
}

/* inicializa la tabla de interrupts */
void initInterrupts() {
  ;
  setInterruptHandler(0x01, kbd_addKeyEvent);
}

/* Inicializa la tabla de syscalls */
void initSyscalls() {
  // Ejemplos de syscalls a registrar:
  // registerSyscall(3, read);
  // registerSyscall(4, write);
  // etc.

  /* SysCalls de teclado */
  registerSyscall(0x10, kbd_pollEvents);
  registerSyscall(0x11, kbd_keydown);
  registerSyscall(0x12, kbd_keypressed);
  registerSyscall(0x13, kbd_keyreleased);
  registerSyscall(0x11, kbd_getKeyEvent);
  /**/
}

/* Registra una syscall */
void registerSyscall(uint64_t id, void *syscall) {
  if (id < MAX_SYSCALLS) syscallDispatchTable[id] = syscall;
}
