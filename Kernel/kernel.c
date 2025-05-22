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

  // uint32_t g = 0;
  // int d = 1;
  // while (1) {
  //   g += d;
  //   if (g == 255 || g == 0) d = -d;
  //
  //   for (uint32_t y = 0; y < VBE_mode_info->height; y++) {
  //     for (uint32_t x = 0; x < VBE_mode_info->width; x++) {
  //       color_t color = ((x * 256 / VBE_mode_info->width) << 16) + (g << 8) +
  //                       (y * 256 / VBE_mode_info->height);
  //       vga_pixel(x, y, color);
  //     }
  //   }
  // }

  vga_clear(0x00000080);

  vga_rect(100, 100, 300, 300, 0xc0c0c0, 0);
  vga_shade(100, 100, 300, 300, 0xff0000, 0);
  vga_frame(100, 100, 300, 300, 0x80ff80, 0);

  for (uint32_t y = 100; y <= 300; y += 25) {
    vga_line(400, y, 600, y, 0xffffff, 0);
  }
  for (uint32_t x = 400; x <= 600; x += 25) {
    vga_line(x, 100, x, 300, 0xffffff, 0);
  }

  for (uint32_t y = 400; y <= 600; y += 25) {
    for (uint32_t x = 100; x <= 300; x += 25) {
      vga_line(200, 500, x, y, 0x00ffff, 0);
    }
  }

  vga_gradient(400, 400, 600, 500, 0xaa5540, 0xc0aa80, VGA_GRAD_H);
  vga_gradient(400, 500, 600, 600, 0xaa5540, 0xc0aa80, VGA_GRAD_V);

  vga_text(700, 100, "Hello world!", 0xff00ff, 0x00ff00, VGA_TEXT_INV);
  vga_text(700, 140, "This is some text\nin multiple lines!", 0xffffff, 0, 0);
  vga_text(
    700, 180, "This\ttext\thas\ttab\tstops\n|\t|\t|\t|\t|", 0xffffff, 0, 0
  );

  const char *longtext =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
    "commodo consequat.";

  vga_textWrap(700, 400, 200, longtext, 0xffffff, 0, VGA_WRAP_WORD);

  vga_rect(700, 400, 800, 600, 0x80ff80ff, VGA_ALPHA_BLEND);

  return 0;
}
