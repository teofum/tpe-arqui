#include "time.h"
#include "vga.h"
#include <interrupts.h>
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
                   PageSize * 8      //The size of the stack itself, 32KiB
                   - sizeof(uint64_t)//Begin at the top of the stack
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
  ncPrint("[Kernel Main]");
  ncNewline();
  ncPrint("  Sample code module at 0x");
  ncPrintHex((uint64_t) sampleCodeModuleAddress);
  ncNewline();
  ncPrint("  Calling the sample code module returned: ");
  ncPrintHex(((EntryPoint) sampleCodeModuleAddress)());
  ncNewline();
  ncNewline();

  ncPrint("  Sample data module at 0x");
  ncPrintHex((uint64_t) sampleDataModuleAddress);
  ncNewline();
  ncPrint("  Sample data module contents: ");
  ncPrint((char *) sampleDataModuleAddress);
  ncNewline();

  initSyscalls();
  setInterruptHandler(0, timer_handler);
  load_idt();

  // Initialize VGA driver
  // vga_gfxMode();
  //
  // // Draw some test graphics
  // vga_clear(0x00);
  //
  // vga_shade(58, 78, 408, 218, 0x00);
  // vga_rect(50, 70, 400, 210, 0x07);
  // vga_frame(50, 70, 400, 210, 0x00);
  //
  // const vga_font_t *lastfont = vga_font(vga_comicsans);
  // vga_text(58, 78, "Hello world!", 0x00, 0);
  // vga_font(lastfont);
  // vga_text(58, 78 + 24, "This is a longer string of text", 0x4c, VGA_TEXT_BG);
  //
  // for (int i = 0; i < 16; i++) {
  //   vga_rect(100 + 20 * i, 400, 100 + 20 * i + 19, 419, i);
  // }
  //
  // vga_setPalette(vga_pal_macintoshii);

  uint32_t g = 0;
  int d = 1;
  while (1) {
    g += d;
    if (g == 255 || g == 0) d = -d;

    for (uint32_t y = 0; y < VBE_mode_info->height; y++) {
      for (uint32_t x = 0; x < VBE_mode_info->width; x++) {
        color_t color = ((x * 256 / VBE_mode_info->width) << 16) + (g << 8) +
                        (y * 256 / VBE_mode_info->height);
        vga_pixel(x, y, color);
      }
    }
  }
  return 0;
}
