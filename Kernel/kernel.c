#include "graphics.h"
#include <interrupts.h>
#include <kbd.h>
#include <lib.h>
#include <moduleLoader.h>
#include <print.h>
#include <status.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <vga.h>

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

// We moved userland a bit to make room for all the framebuffers
static void *const userlandCodeModule = (void *) 0x2000000;

// Data module addresses
static void *const gameLogoDataModule = (void *) 0x3000000;// Image, ~10K
static void *const modelDataModule1 = (void *) 0x3003000;// Capybara model, ~10K
static void *const modelDataModule2 = (void *) 0x3006000;// Capybara face, ~1K
static void *const modelDataModule3 = (void *) 0x3006800;// Golf club, 744B
static void *const modelDataModule4 = (void *) 0x3006C00;// Flag, 216B
static void *const modelDataModule5 = (void *) 0x3006E00;// Flagpole, 420B
static void *const modelDataModule6 = (void *) 0x3007000;// Ball, 780B
static void *const modelDataModule7 = (void *) 0x3007800;// Utah teapot, ~58K!!

typedef int (*entrypoint_t)();

void clearBSS(void *bssAddress, uint64_t bssSize) {
  memset(bssAddress, 0, bssSize);
}

void *getStackBase() {
  return (void *) ((uint64_t) &endOfKernel +
                   PageSize * 8      //The size of the stack itself, 32KiB
                   - sizeof(uint64_t)//Begin at the top of the stack
  );
}

void *initializeKernelBinary() {
  void *moduleAddresses[] = {
    userlandCodeModule, gameLogoDataModule, modelDataModule1,
    modelDataModule2,   modelDataModule3,   modelDataModule4,
    modelDataModule5,   modelDataModule6,   modelDataModule7,
  };
  loadModules(&endOfKernelBinary, moduleAddresses);

  clearBSS(&bss, &endOfKernel - &bss);

  return getStackBase();
}

int main() {
  // Initialize interrupts and syscalls
  initSyscalls();
  initInterrupts();
  loadIDT();

  // Init timer
  timer_init();

  // Initialize video driver and graphics
  vga_init();
  gfx_init();

  // Enable status bar
  status_setEnabled(1);

  while (1) {
    int ret = ((entrypoint_t) userlandCodeModule)();
    printf(
      "\x1A 195,248,132;[Kernel] \x1A R;"
      "Userland module exited with code \x1A 255,197,96;%#08x\n"
      "\x1A 195,248,132;[Kernel] \x1A R;"
      "Press any key to restart shell\n",
      ret
    );

    int key = 0;
    while (!key) { key = kbd_getKeyEvent().key; }
  }

  return 0;
}
