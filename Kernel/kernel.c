#include <graphics.h>
#include <interrupts.h>
#include <io.h>
#include <kbd.h>
#include <lib.h>
#include <mem.h>
#include <module_loader.h>
#include <print.h>
#include <process.h>
#include <status.h>
#include <stdint.h>
#include <time.h>
#include <vga.h>

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t end_of_kernel_binary;
extern uint8_t end_of_kernel;

static const uint64_t page_size = 0x1000;

static void *const userland_code_module = (void *) 0x400000;

// Data module addresses
static void *const game_logo_data_module = (void *) 0x800000;// Image, ~10K
static void *const model_data_module_1 =
  (void *) 0x803000;// Capybara model, ~10K
static void *const model_data_module_2 = (void *) 0x806000;// Capybara face, ~1K
static void *const model_data_module_3 = (void *) 0x806800;// Golf club, 744B
static void *const model_data_module_4 = (void *) 0x806C00;// Flag, 216B
static void *const model_data_module_5 = (void *) 0x806E00;// Flagpole, 420B
static void *const model_data_module_6 = (void *) 0x807000;// Ball, 780B
static void *const model_data_module_7 =
  (void *) 0x807800;// Utah teapot, ~58K!!

typedef int (*entrypoint_t)();

void clear_bss(void *bss_address, uint64_t bss_size) {
  memset(bss_address, 0, bss_size);
}

void *get_stack_base() {
  return (void *) ((uint64_t) &end_of_kernel +
                   page_size * 8      //The size of the stack itself, 32KiB
                   - sizeof(uint64_t)//Begin at the top of the stack
  );
}

void *initialize_kernel_binary() {
  void *module_addresses[] = {
    userland_code_module, game_logo_data_module, model_data_module_1,
    model_data_module_2,  model_data_module_3,   model_data_module_4,
    model_data_module_5,  model_data_module_6,   model_data_module_7,
  };
  load_modules(&end_of_kernel_binary, module_addresses);

  clear_bss(&bss, &end_of_kernel - &bss);

  return get_stack_base();
}

void test_b() {
  while (1) { io_write("B", 1); }
}

void test_a() {
  for (int i = 0; i < 10; i++) { io_write("A", 1); }
  proc_spawn(test_b);
  while (1) { io_write("A", 1); }
}

void init() {
  proc_spawn(test_a);
  proc_spawn(test_b);
}

int main() {
  // Initialize memory manager
  mem_default_mgr =
    mem_manager_create((void *) 0xFFF000, (void *) 0x1000000, 0);

  proc_kernel_stack = mem_alloc(4096);

  // Init timer
  timer_init();

  // Initialize video driver and graphics
  vga_init();
  gfx_init();

  // Initialize IO
  io_init();

  // Enable status bar
  status_set_enabled(1);

  // Initialize interrupts and syscalls
  init_syscalls();
  init_interrupts();
  load_idt();

  printf("Stack at %#016lx\n\n", (size_t) get_stack_base());

  while (1) {
    proc_init(test_a);
    // int ret = ((entrypoint_t) userland_code_module)();
    // printf(
    //   "\x1A 195,248,132;[Kernel] \x1A R;"
    //   "Userland module exited with code \x1A 255,197,96;%#08x\n"
    //   "\x1A 195,248,132;[Kernel] \x1A R;"
    //   "Press any key to restart shell\n",
    //   ret
    // );

    int key = 0;
    while (!key) { key = kbd_get_key_event().key; }
  }

  return 0;
}
