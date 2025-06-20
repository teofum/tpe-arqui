#include <audio.h>
#include <graphics.h>
#include <interrupts.h>
#include <io.h>
#include <kbd.h>
#include <lib.h>
#include <print.h>
#include <status.h>
#include <stdint.h>
#include <time.h>
#include <vga.h>

/* Flags para derechos de acceso de los segmentos */
#define ACS_PRESENT 0x80 /* segmento presente en memoria */
#define ACS_CSEG 0x18    /* segmento de codigo */
#define ACS_DSEG 0x10    /* segmento de datos */
#define ACS_READ 0x02    /* segmento de lectura */
#define ACS_WRITE 0x02   /* segmento de escritura */
#define ACS_IDT ACS_DSEG
#define ACS_INT_386 0x0E /* Interrupt GATE 32 bits */
#define ACS_INT (ACS_PRESENT | ACS_INT_386)

#define ACS_CODE (ACS_PRESENT | ACS_CSEG | ACS_READ)
#define ACS_DATA (ACS_PRESENT | ACS_DSEG | ACS_WRITE)
#define ACS_STACK (ACS_PRESENT | ACS_DSEG | ACS_WRITE)

#define ID_TIMER_TICK 0x20
#define ID_KEYBOARD 0x21
#define ID_SYSCALL 0x80

#define MAX_INTERRUPTS 256
#define MAX_SYSCALLS 256
#define MAX_EXCEPTIONS 256

#define REGDUMP_NORMAL 0x00
#define REGDUMP_EXCEPTION 0x01

#define TEXT_COLOR 0xffffff
#define FRAME_COLOR 0xffffff

#define MEM_DUMP_SIZE 128

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
  uint64_t cr0, cr2, cr3, cr4, cr8;
  uint16_t cs, ss, ds, es, fs, gs;
} registerState;

extern void _picMasterMask(uint8_t mask);
extern void _picSlaveMask(uint8_t mask);

extern void _irq00Handler();
extern void _irq01Handler();
extern void _irq80Handler();

extern void _exception00Handler();
extern void _exception06Handler();

extern void outb(uint16_t port, uint8_t value);

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

  // Exceptions
  // Division by Zero (00)
  setup_IDT_Entry(0x00, (uint64_t) &_exception00Handler);
  // Invalid Opcode (06)
  setup_IDT_Entry(0x06, (uint64_t) &_exception06Handler);

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
  // Keyboard interrupt (0x01) is handled specially for register dump function,
  // so it doesn't have an entry in the interrupt dispatch table
}

/* Registra una syscall */
static void registerSyscall(uint64_t id, void *syscall) {
  syscallDispatchTable[id] = syscall;
}

/* Inicializa la tabla de syscalls */
void initSyscalls() {
  /* Virtual terminal I/O */
  registerSyscall(0x03, io_read);
  registerSyscall(0x04, io_write);
  // 0x05, 0x06 reserved for future syscalls (open, close)
  registerSyscall(0x07, io_writes);
  registerSyscall(0x08, io_putc);
  registerSyscall(0x09, io_clear);
  registerSyscall(0x0A, io_setfont);
  registerSyscall(0x0B, io_blankFrom);
  registerSyscall(0x0C, io_setcursor);
  registerSyscall(0x0D, io_movecursor);

  /* Keyboard */
  registerSyscall(0x10, kbd_pollEvents);
  registerSyscall(0x11, kbd_keydown);
  registerSyscall(0x12, kbd_keypressed);
  registerSyscall(0x13, kbd_keyreleased);
  registerSyscall(0x14, kbd_getKeyEvent);
  registerSyscall(0x15, kbd_getchar);

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
  registerSyscall(0x2A, vga_present);
  registerSyscall(0x2B, vga_setFramebuffer);
  registerSyscall(0x2C, vga_copy);
  registerSyscall(0x2D, vga_copy2x);
  registerSyscall(0x2E, vga_bitmap);
  registerSyscall(0x2F, vga_getVBEInfo);

  /* Audio */
  registerSyscall(0x30, audio_beep);
  registerSyscall(0x31, audio_play_melody);

  /* Status bar */
  registerSyscall(0x40, status_enabled);
  registerSyscall(0x41, status_setEnabled);

  /* Time/RTC */
  registerSyscall(0x50, ticks_elapsed);

  /* Graphics module */
  registerSyscall(0xA0, gfx_clear);
  registerSyscall(0xA1, gfx_drawPrimitives);
  registerSyscall(0xA2, gfx_drawPrimitivesIndexed);
  registerSyscall(0xA3, gfx_drawWireframe);
  registerSyscall(0xA4, gfx_drawWireframeIndexed);
  registerSyscall(0xA5, gfx_setBuffers);
  registerSyscall(0xA6, gfx_copy);
  registerSyscall(0xA7, gfx_depthcopy);
  registerSyscall(0xA8, gfx_loadModel);
  registerSyscall(0xAA, gfx_setLight);
  registerSyscall(0xAB, gfx_setLightType);
  registerSyscall(0xAC, gfx_setMatrix);
  registerSyscall(0xAD, gfx_setFlag);
  registerSyscall(0xAE, gfx_getFramebuffer);
  registerSyscall(0xAF, gfx_present);

  /* Special */
  registerSyscall(0xFF, _hlt);
}

// Información de excepción
typedef struct {
  const char *name;
  const char *description;
} exception_info_t;

// Tabla de información de excepciones - índice directo por ID
static const exception_info_t exceptionTable[MAX_EXCEPTIONS] = {
  [0x00] = {"Division by Zero", "Attempt to divide by zero"},
  [0x06] = {"Invalid Opcode", "Illegal or undefined instruction"}
};

typedef struct {
  uint64_t flag;
  uint64_t exception_id;

  uint64_t memdumpStart;
  uint8_t memdump[MEM_DUMP_SIZE];
} regdump_context_t;

regdump_context_t regdumpContext = {REGDUMP_NORMAL, 0};

void showCPUState() {
  uint16_t height, top;
  uint16_t width = 800, left = CENTER_X - (width >> 1);
  uint16_t bottom, right = left + width - 1;
  uint64_t bg_colors;
  const char *footer;
  char buf[256];

  int statusEnabled = status_enabled();
  vga_framebuffer_t currentFB = vga_setFramebuffer(NULL);
  if (regdumpContext.flag == REGDUMP_EXCEPTION) {
    status_setEnabled(0);
    bg_colors = colors(0x500000, 0x800000);
    footer = "Press any key to restart";

    height = 560;
    top = (VGA_HEIGHT - height) >> 1;
    bottom = top + height - 1;

    vga_gradient(left, top, right, bottom, bg_colors, VGA_GRAD_V);
    vga_frame(left, top, right, bottom, FRAME_COLOR, 0);

    vga_text(
      CENTER_X - 80, top + 16, " !! KERNEL PANIC !! ", 0, TEXT_COLOR,
      VGA_TEXT_INV
    );

    top += 32;

    vga_text(
      CENTER_X - 192, top + 16,
      "Execution halted due to an unexpected exception.", TEXT_COLOR, 0, 0
    );
    vga_text(CENTER_X - 72, top + 32, "CPU state dumped.", TEXT_COLOR, 0, 0);

    vga_text(
      CENTER_X - 92, top + 64, "== Exception Details ==", 0, TEXT_COLOR,
      VGA_TEXT_INV
    );

    top += 80;

    // Información específica de la excepción
    const exception_info_t *info = &exceptionTable[regdumpContext.exception_id];
    if (info->name) {
      sprintf(
        buf, "Exception %#02llx %s", regdumpContext.exception_id, info->name
      );
      vga_text(left + 16, top + 16, buf, TEXT_COLOR, 0, 0);

      sprintf(buf, "Description: %s", info->description);
      vga_text(left + 16, top + 32, buf, TEXT_COLOR, 0, 0);
    } else {
      sprintf(buf, "Exception %#02llx", regdumpContext.exception_id);
      vga_text(left + 16, top + 16, buf, TEXT_COLOR, 0, 0);
      vga_text(
        left + 16, top + 32, "Description: Unknown exception", TEXT_COLOR, 0, 0
      );
    }

    vga_text(
      CENTER_X - 80, top + 64, "== CPU state dump ==", 0, TEXT_COLOR,
      VGA_TEXT_INV
    );

    top += 80;
  } else {
    bg_colors = colors(0x0020a0, 0x2040c0);
    footer = "Press any key to continue";

    height = 208;
    top = (VGA_HEIGHT - height) >> 1;
    bottom = top + height - 1;

    vga_gradient(left, top, right, bottom, bg_colors, VGA_GRAD_V);
    vga_frame(left, top, right, bottom, FRAME_COLOR, 0);

    vga_text(
      CENTER_X - 80, top + 16, "== CPU state dump ==", 0, TEXT_COLOR,
      VGA_TEXT_INV
    );

    top += 32;
  }

  /* General purpose registers */
  sprintf(buf, "rax: %#016llx", registerState.rax);
  vga_text(left + 8, top + 16, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rbx: %#016llx", registerState.rbx);
  vga_text(left + 8, top + 32, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rcx: %#016llx", registerState.rcx);
  vga_text(left + 8, top + 48, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rdx: %#016llx", registerState.rdx);
  vga_text(left + 8, top + 64, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rdi: %#016llx", registerState.rdi);
  vga_text(left + 208, top + 16, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rsi: %#016llx", registerState.rsi);
  vga_text(left + 208, top + 32, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rsp: %#016llx", registerState.rsp);
  vga_text(left + 208, top + 48, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rbp: %#016llx", registerState.rbp);
  vga_text(left + 208, top + 64, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, " r8: %#016llx", registerState.r8);
  vga_text(left + 408, top + 16, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, " r9: %#016llx", registerState.r9);
  vga_text(left + 408, top + 32, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r10: %#016llx", registerState.r10);
  vga_text(left + 408, top + 48, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r11: %#016llx", registerState.r11);
  vga_text(left + 408, top + 64, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r12: %#016llx", registerState.r12);
  vga_text(left + 608, top + 16, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r13: %#016llx", registerState.r13);
  vga_text(left + 608, top + 32, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r14: %#016llx", registerState.r14);
  vga_text(left + 608, top + 48, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r15: %#016llx", registerState.r15);
  vga_text(left + 608, top + 64, buf, TEXT_COLOR, 0, 0);

  /* Execution state registers */
  sprintf(buf, "rip: %#016llx", registerState.rip);
  vga_text(left + 8, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "flg: %#016llx", registerState.rflags);
  vga_text(left + 8, top + 112, buf, TEXT_COLOR, 0, 0);

  /* Segment registers */
  sprintf(buf, "cs: %#04x", registerState.cs);
  vga_text(left + 208, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "ss: %#04x", registerState.ss);
  vga_text(left + 312, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "ds: %#04x", registerState.ds);
  vga_text(left + 408, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "es: %#04x", registerState.es);
  vga_text(left + 512, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "fs: %#04x", registerState.fs);
  vga_text(left + 608, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "gs: %#04x", registerState.gs);
  vga_text(left + 712, top + 96, buf, TEXT_COLOR, 0, 0);

  top += 144;

  /* Memory dump (exception only) */
  if (regdumpContext.flag == REGDUMP_EXCEPTION) {
    vga_text(
      CENTER_X - 140, top, "== Memory dump [RIP highlighted] ==", 0, TEXT_COLOR,
      VGA_TEXT_INV
    );

    for (uint32_t y = 0; y < 8; y++) {
      sprintf(buf, "%#016llx", regdumpContext.memdumpStart + y * 16);
      vga_text(CENTER_X - 272, top + 32 + y * 16, buf, TEXT_COLOR, 0, 0);

      for (uint32_t x = 0; x < 16; x++) {
        sprintf(buf, "%02x", regdumpContext.memdump[y * 16 + x]);
        vga_text(
          CENTER_X - 104 + x * 24, top + 32 + y * 16, buf, TEXT_COLOR,
          TEXT_COLOR,
          regdumpContext.memdumpStart + y * 16 + x == registerState.rip
            ? VGA_TEXT_INV
            : 0
        );
      }
    }

    top += 192;
  }

  /* Prompt */
  uint32_t offset = regdumpContext.flag == REGDUMP_EXCEPTION ? 96 : 100;
  vga_text(CENTER_X - offset, top, footer, TEXT_COLOR, 0, 0);

  vga_present();
  vga_setFramebuffer(currentFB);

  char key = 0;
  while (!key) { key = kbd_getKeyEvent().key; }

  // Restore statusbar
  status_setEnabled(statusEnabled);

  // Restore regdump ctx
  regdumpContext.flag = REGDUMP_NORMAL;
  regdumpContext.exception_id = 0;
}
