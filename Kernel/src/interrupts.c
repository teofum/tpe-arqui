#include <audio.h>
#include <graphics.h>
#include <interrupts.h>
#include <io.h>
#include <kbd.h>
#include <lib.h>
#include <mem.h>
#include <print.h>
#include <process.h>
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

;// empty statement, mutes warning from clangd bug (https://github.com/clangd/clangd/issues/1167)
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
void (*irq_handlers[MAX_INTERRUPTS])();
void *syscall_dispatch_table[MAX_SYSCALLS];

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
} register_state;

extern void _pic_master_mask(uint8_t mask);
extern void _pic_slave_mask(uint8_t mask);

extern void _irq_00_handler();
extern void _irq_01_handler();
extern void _irq_80_handler();

extern void _exception_00_handler();
extern void _exception_06_handler();

extern void outb(uint16_t port, uint8_t value);

/* Configura una entrada en la IDT */
static void setup_idt_entry(int index, uint64_t offset) {
  idt[index].selector = 0x08;
  idt[index].offset_l = offset & 0xFFFF;
  idt[index].offset_m = (offset >> 16) & 0xFFFF;
  idt[index].offset_h = (offset >> 32) & 0xFFFFFFFF;
  idt[index].access = ACS_INT;
  idt[index].zero = 0;
  idt[index].other_zero = (uint64_t) 0;
}

/* Carga la IDT con las interrupciones configuradas */
void load_idt() {
  _cli();

  // IRQ 0: Timer tick
  setup_idt_entry(ID_TIMER_TICK, (uint64_t) &_irq_00_handler);

  // IRQ 1: Keyboard
  setup_idt_entry(ID_KEYBOARD, (uint64_t) &_irq_01_handler);

  // Syscalls
  setup_idt_entry(ID_SYSCALL, (uint64_t) &_irq_80_handler);

  // Exceptions
  // Division by Zero (00)
  setup_idt_entry(0x00, (uint64_t) &_exception_00_handler);
  // Invalid Opcode (06)
  setup_idt_entry(0x06, (uint64_t) &_exception_06_handler);

  _pic_master_mask(0xFC);
  _pic_slave_mask(0xFF);

  _sti();
}

/*
 * Interrupt handler for IRQ 0 (timer)
 */
uint64_t irq_timer_handler(uint64_t running_process_rsp) {
  // Store stack pointer for the current process
  proc_control_table[proc_running_pid].rsp = running_process_rsp;

  // Handle timer interrupt, may call the scheduler and cause a process switch
  timer_handler();

  // Return stack pointer for the now running process
  return proc_control_table[proc_running_pid].rsp;
}

static void register_syscall(uint64_t id, void *syscall) {
  syscall_dispatch_table[id] = syscall;
}

void init_syscalls() {
  /* Virtual terminal I/O */
  register_syscall(0x03, io_read);
  register_syscall(0x04, io_write);
  // 0x05, 0x06 reserved for future syscalls (open, close)
  register_syscall(0x07, io_writes);
  register_syscall(0x09, io_clear);
  register_syscall(0x0A, io_setfont);
  register_syscall(0x0B, io_blank_from);
  register_syscall(0x0C, io_setcursor);
  register_syscall(0x0D, io_movecursor);

  /* Keyboard */
  register_syscall(0x10, kbd_poll_events);
  register_syscall(0x11, kbd_keydown);
  register_syscall(0x12, kbd_keypressed);
  register_syscall(0x13, kbd_keyreleased);
  register_syscall(0x14, kbd_get_key_event);
  register_syscall(0x15, kbd_getchar);

  /* Video */
  register_syscall(0x20, vga_clear);
  register_syscall(0x21, vga_pixel);
  register_syscall(0x22, vga_line);
  register_syscall(0x23, vga_rect);
  register_syscall(0x24, vga_frame);
  register_syscall(0x25, vga_shade);
  register_syscall(0x26, vga_gradient);
  register_syscall(0x27, vga_font);
  register_syscall(0x28, vga_text);
  register_syscall(0x29, vga_text_wrap);
  register_syscall(0x2A, vga_present);
  register_syscall(0x2B, vga_set_framebuffer);
  register_syscall(0x2C, vga_copy);
  register_syscall(0x2D, vga_copy_ex);
  register_syscall(0x2E, vga_bitmap);
  register_syscall(0x2F, vga_get_vbe_info);

  /* Audio */
  register_syscall(0x30, audio_beep);
  register_syscall(0x31, audio_play_melody);

  /* Status bar */
  register_syscall(0x40, status_enabled);
  register_syscall(0x41, status_set_enabled);

  /* Time/RTC */
  register_syscall(0x50, ticks_elapsed);

  /* Memory allocation */
  register_syscall(0x60, mem_alloc);
  register_syscall(0x61, mem_free);
  register_syscall(0x62, mem_check);
  register_syscall(0x63, mem_status);

  /* Processes */
  register_syscall(0x70, proc_spawn);
  register_syscall(0x71, proc_exit);
  register_syscall(0x72, proc_wait);
  register_syscall(0x73, proc_getpid);
  register_syscall(0x74, proc_wait_for_foreground);
  register_syscall(0x75, proc_info);
  register_syscall(0x76, proc_kill);
  register_syscall(0x77, proc_yield);
  register_syscall(0x78, proc_blockpid);
  register_syscall(0x79, proc_runpid);

  /* Graphics module */
  register_syscall(0xA0, gfx_clear);
  register_syscall(0xA1, gfx_draw_primitives);
  register_syscall(0xA2, gfx_draw_primitives_indexed);
  register_syscall(0xA3, gfx_draw_wireframe);
  register_syscall(0xA4, gfx_draw_wireframe_indexed);
  register_syscall(0xA5, gfx_set_buffers);
  register_syscall(0xA6, gfx_copy);
  register_syscall(0xA7, gfx_depthcopy);
  register_syscall(0xA8, gfx_load_model);
  register_syscall(0xAA, gfx_set_light);
  register_syscall(0xAB, gfx_set_light_type);
  register_syscall(0xAC, gfx_set_matrix);
  register_syscall(0xAD, gfx_set_flag);
  register_syscall(0xAE, gfx_get_framebuffer);
  register_syscall(0xAF, gfx_present);
}

typedef struct {
  const char *name;
  const char *description;
} exception_info_t;

static const exception_info_t exception_table[MAX_EXCEPTIONS] = {
  [0x00] = {"Division by Zero", "Attempt to divide by zero"},
  [0x06] = {"Invalid Opcode", "Illegal or undefined instruction"}
};

typedef struct {
  uint64_t flag;
  uint64_t exception_id;

  uint64_t memdump_start;
  uint8_t memdump[MEM_DUMP_SIZE];
} regdump_context_t;

regdump_context_t regdump_context = {REGDUMP_NORMAL, 0};

void show_cpu_state() {
  uint16_t height, top;
  uint16_t width = 800, left = CENTER_X - (width >> 1);
  uint16_t bottom, right = left + width - 1;
  uint64_t bg_colors;
  const char *footer;
  char buf[256];

  int is_status_enabled = status_enabled();
  vga_framebuffer_t current_fb = vga_set_framebuffer(NULL);
  if (regdump_context.flag == REGDUMP_EXCEPTION) {
    status_set_enabled(0);
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
    const exception_info_t *info =
      &exception_table[regdump_context.exception_id];
    if (info->name) {
      sprintf(
        buf, "Exception %#02llx %s", regdump_context.exception_id, info->name
      );
      vga_text(left + 16, top + 16, buf, TEXT_COLOR, 0, 0);

      sprintf(buf, "Description: %s", info->description);
      vga_text(left + 16, top + 32, buf, TEXT_COLOR, 0, 0);
    } else {
      sprintf(buf, "Exception %#02llx", regdump_context.exception_id);
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
  sprintf(buf, "rax: %#016llx", register_state.rax);
  vga_text(left + 8, top + 16, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rbx: %#016llx", register_state.rbx);
  vga_text(left + 8, top + 32, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rcx: %#016llx", register_state.rcx);
  vga_text(left + 8, top + 48, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rdx: %#016llx", register_state.rdx);
  vga_text(left + 8, top + 64, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rdi: %#016llx", register_state.rdi);
  vga_text(left + 208, top + 16, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rsi: %#016llx", register_state.rsi);
  vga_text(left + 208, top + 32, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rsp: %#016llx", register_state.rsp);
  vga_text(left + 208, top + 48, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "rbp: %#016llx", register_state.rbp);
  vga_text(left + 208, top + 64, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, " r8: %#016llx", register_state.r8);
  vga_text(left + 408, top + 16, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, " r9: %#016llx", register_state.r9);
  vga_text(left + 408, top + 32, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r10: %#016llx", register_state.r10);
  vga_text(left + 408, top + 48, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r11: %#016llx", register_state.r11);
  vga_text(left + 408, top + 64, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r12: %#016llx", register_state.r12);
  vga_text(left + 608, top + 16, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r13: %#016llx", register_state.r13);
  vga_text(left + 608, top + 32, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r14: %#016llx", register_state.r14);
  vga_text(left + 608, top + 48, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "r15: %#016llx", register_state.r15);
  vga_text(left + 608, top + 64, buf, TEXT_COLOR, 0, 0);

  /* Execution state registers */
  sprintf(buf, "rip: %#016llx", register_state.rip);
  vga_text(left + 8, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "flg: %#016llx", register_state.rflags);
  vga_text(left + 8, top + 112, buf, TEXT_COLOR, 0, 0);

  /* Segment registers */
  sprintf(buf, "cs: %#04x", register_state.cs);
  vga_text(left + 208, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "ss: %#04x", register_state.ss);
  vga_text(left + 312, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "ds: %#04x", register_state.ds);
  vga_text(left + 408, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "es: %#04x", register_state.es);
  vga_text(left + 512, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "fs: %#04x", register_state.fs);
  vga_text(left + 608, top + 96, buf, TEXT_COLOR, 0, 0);

  sprintf(buf, "gs: %#04x", register_state.gs);
  vga_text(left + 712, top + 96, buf, TEXT_COLOR, 0, 0);

  top += 144;

  /* Memory dump (exception only) */
  if (regdump_context.flag == REGDUMP_EXCEPTION) {
    vga_text(
      CENTER_X - 140, top, "== Memory dump [RIP highlighted] ==", 0, TEXT_COLOR,
      VGA_TEXT_INV
    );

    for (uint32_t y = 0; y < 8; y++) {
      sprintf(buf, "%#016llx", regdump_context.memdump_start + y * 16);
      vga_text(CENTER_X - 272, top + 32 + y * 16, buf, TEXT_COLOR, 0, 0);

      for (uint32_t x = 0; x < 16; x++) {
        sprintf(buf, "%02x", regdump_context.memdump[y * 16 + x]);
        vga_text(
          CENTER_X - 104 + x * 24, top + 32 + y * 16, buf, TEXT_COLOR,
          TEXT_COLOR,
          regdump_context.memdump_start + y * 16 + x == register_state.rip
            ? VGA_TEXT_INV
            : 0
        );
      }
    }

    top += 192;
  }

  /* Prompt */
  uint32_t offset = regdump_context.flag == REGDUMP_EXCEPTION ? 96 : 100;
  vga_text(CENTER_X - offset, top, footer, TEXT_COLOR, 0, 0);

  vga_present();
  vga_set_framebuffer(current_fb);

  char key = 0;
  while (!key) { key = kbd_get_key_event().key; }

  // Restore statusbar
  status_set_enabled(is_status_enabled);

  // Restore regdump ctx
  regdump_context.flag = REGDUMP_NORMAL;
  regdump_context.exception_id = 0;
}
