#include "vga.h"
#include <defs.h>
#include <interrupts.h>
#include <kbd.h>
#include <stdint.h>
#include <time.h>

#define ID_TIMER_TICK 0x20
#define ID_KEYBOARD 0x21
#define ID_SYSCALL 0x80

#define MAX_INTERRUPTS 256
#define MAX_SYSCALLS 256

#pragma pack(push) /* Push de la alineación actual */
#pragma pack(1)    /* Alinear las siguiente estructuras a 1 byte */

typedef struct {
  uint16_t offset_l, selector;
  uint8_t zero, access;
  uint16_t offset_m;
  uint32_t offset_h, other_zero;
} idt_descriptor_t;

#pragma pack(pop) /* Reestablece la alineación actual */

idt_descriptor_t *idt = (idt_descriptor_t *) 0;
void (*irqHandlers[MAX_INTERRUPTS])();
void *syscallDispatchTable[MAX_SYSCALLS];

/*
 * Static memory location for CPU state dump
 */
struct {
  uint64_t rax, rbx, rcx, rdx;
  uint64_t rsi, rdi, rsp, rbp;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t rip, rflags;
  uint16_t cs, ss, ds, es, fs, gs;
  uint64_t cr0, cr2, cr3, cr4, cr8;
} registerState;

extern void _picMasterMask(uint8_t mask);
extern void _picSlaveMask(uint8_t mask);
extern void _cli(void);
extern void _sti(void);

extern void _irq00Handler();
extern void _irq01Handler();
extern void _irq80Handler();

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

/* Carga la IDT con las interrupciones configuradas */
void loadIDT() {
  _cli();

  // IRQ 0: Timer tick
  setup_IDT_Entry(ID_TIMER_TICK, (uint64_t) &_irq00Handler);

  // IRQ 1: Teclado (comentado por ahora)
  setup_IDT_Entry(ID_KEYBOARD, (uint64_t) &_irq01Handler);

  // Syscalls
  setup_IDT_Entry(ID_SYSCALL, (uint64_t) &_irq80Handler);

  _picMasterMask(0xFC);
  _picSlaveMask(0xFF);

  _sti();
}

/* Dispatcher de IRQs */
void irqDispatcher(uint64_t irq) { irqHandlers[irq](); }

/* Registra un handler para una IRQ */
static void setInterruptHandler(uint64_t irq, void (*handler)()) {
  irqHandlers[irq] = handler;
}

/* inicializa la tabla de interrupts */
void initInterrupts() {
  setInterruptHandler(0x00, timer_handler);
  setInterruptHandler(0x01, kbd_addKeyEvent);
}

/* Registra una syscall */
static void registerSyscall(uint64_t id, void *syscall) {
  syscallDispatchTable[id] = syscall;
}

/* Inicializa la tabla de syscalls */
void initSyscalls() {
  /* Keyboard */
  registerSyscall(0x10, kbd_pollEvents);
  registerSyscall(0x11, kbd_keydown);
  registerSyscall(0x12, kbd_keypressed);
  registerSyscall(0x13, kbd_keyreleased);
  registerSyscall(0x11, kbd_getKeyEvent);

  /* Video */
  registerSyscall(0x20, vga_clear);
  registerSyscall(0x21, vga_pixel);
  registerSyscall(0x22, vga_line);
  registerSyscall(0x23, vga_rect);
  registerSyscall(0x24, vga_frame);
  registerSyscall(0x25, vga_shade);
  registerSyscall(0x26, vga_gradient);
  registerSyscall(0x27, vga_font);
  registerSyscall(0x28, vga_text);
  registerSyscall(0x29, vga_textWrap);
}
