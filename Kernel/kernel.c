#include "status.h"
#include "time.h"
#include <audio.h>
#include <interrupts.h>
#include <io.h>
#include <kbd.h>
#include <lib.h>
#include <moduleLoader.h>
#include <print.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vga.h>

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void *const sampleCodeModuleAddress = (void *) 0x2000000;
static void *const sampleDataModuleAddress = (void *) 0x3000000;

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
  void *moduleAddresses[] = {sampleCodeModuleAddress, sampleDataModuleAddress};
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

  // Initialize video driver
  vga_init();

  // Enable status bar
  status_setEnabled(1);

  while (1) {
    int ret = ((entrypoint_t) sampleCodeModuleAddress)();
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
