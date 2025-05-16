#include "vga.h"
#include <lib.h>
#include <moduleLoader.h>
#include <naiveConsole.h>
#include <stdint.h>
#include <string.h>

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void *const sampleCodeModuleAddress = (void *) 0x400000;
static void *const sampleDataModuleAddress = (void *) 0x500000;

typedef int (*EntryPoint)();

void clearBSS(void *bssAddress, uint64_t bssSize) {
  memset(bssAddress, 0, bssSize);
}

void *getStackBase() {
  return (void *) ((uint64_t) &endOfKernel +
                   PageSize * 8      // The size of the stack itself, 32KiB
                   - sizeof(uint64_t)// Begin at the top of the stack
  );
}

void *initializeKernelBinary() {
  char buffer[10];

  ncPrint("[x64BareBones]");
  ncNewline();

  ncPrint("CPU Vendor:");
  ncPrint(cpuVendor(buffer));
  ncNewline();

  ncPrint("[Loading modules]");
  ncNewline();
  void *moduleAddresses[] = {sampleCodeModuleAddress, sampleDataModuleAddress};

  loadModules(&endOfKernelBinary, moduleAddresses);
  ncPrint("[Done]");
  ncNewline();
  ncNewline();

  ncPrint("[Initializing kernel's binary]");
  ncNewline();

  clearBSS(&bss, &endOfKernel - &bss);

  ncPrint("  text: 0x");
  ncPrintHex((uint64_t) &text);
  ncNewline();
  ncPrint("  rodata: 0x");
  ncPrintHex((uint64_t) &rodata);
  ncNewline();
  ncPrint("  data: 0x");
  ncPrintHex((uint64_t) &data);
  ncNewline();
  ncPrint("  bss: 0x");
  ncPrintHex((uint64_t) &bss);
  ncNewline();

  ncPrint("[Done]");
  ncNewline();
  ncNewline();
  return getStackBase();
}

int main() {
  // Initialize VGA driver
  vga_gfxMode();

  // Draw some test graphics
  vga_clear(0x01);

  // vga_pixel(128, 10, 0xc);
  // vga_pixel(128, 20, 0xc);
  // vga_pixel(128, 30, 0xc);
  // vga_pixel(128, 40, 0xc);
  // vga_pixel(128, 50, 0xc);
  // vga_pixel(128, 60, 0xc);
  // vga_pixel(128, 70, 0xc);
  //
  // vga_line(0, 10, 127, 0, 0x07);
  // vga_line(0, 20, 128, 0, 0x07);
  // vga_line(20, 30, 127, 0, 0x07);
  // vga_line(0, 40, 150, 0, 0x07);
  // vga_line(0, 50, 40, 0, 0x07);
  // vga_line(20, 60, 40, 0, 0x07);

  // Slow: fill screen drawing individual pixels
  for (int y = 0; y < VGA_HEIGHT; y++)
    for (int x = 0; x < VGA_WIDTH; x++) vga_pixel(x, y, 0x02);

  // fast as fuk boiiii
  for (int y = 0; y < VGA_HEIGHT; y++) vga_line(0, y, VGA_WIDTH - 1, y, 0x04);

  // vga_textMode();

  return 0;
}
