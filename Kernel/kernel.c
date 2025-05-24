#include "kbd.h"
#include "vga.h"
#include <interrupts.h>
#include <kbd.h>
#include <lib.h>
#include <moduleLoader.h>
#include <naiveConsole.h>
#include <stdint.h>
#include <string.h>
#include <time.h>


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
  initInterrupts();
  loadIDT();

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

  vga_text(700, 100, "This is some text\nin multiple lines!", 0xffffff, 0, 0);
  vga_text(
    700, 132, "This\ttext\thas\ttab\tstops\n|\t|\t|\t|\t|", 0xffffff, 0, 0
  );

  const vga_font_t *font = vga_font(vga_fontTiny);
  vga_text(700, 174, "Hello world!", 0xffffff, 0, 0);
  vga_font(vga_fontTinyBold);
  vga_text(700, 174 + 8, "Hello world!", 0xffffff, 0, 0);
  vga_font(vga_fontSmall);
  vga_text(700, 178 + 16, "Hello world!", 0xffffff, 0, 0);
  vga_font(vga_fontDefault);
  vga_text(700, 178 + 28, "Hello world!", 0xffffff, 0, 0);
  vga_font(vga_fontLarge);
  vga_text(700, 178 + 44, "Hello world!", 0xffffff, 0, 0);
  vga_font(vga_fontAlt);
  vga_text(700, 182 + 68, "Hello world!", 0xffffff, 0, 0);
  vga_font(vga_fontAltBold);
  vga_text(700, 182 + 84, "Hello world!", 0xffffff, 0, 0);
  vga_font(vga_fontFuture);
  vga_text(700, 186 + 100, "Hello world!", 0xffffff, 0, 0);
  vga_font(vga_fontOld);
  vga_text(700, 186 + 110, "Hello world!", 0xffffff, 0, 0);
  vga_font(font);

  const char *longtext =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
    "commodo consequat.";

  vga_textWrap(700, 400, 200, longtext, 0xffffff, 0, VGA_WRAP_WORD);

  vga_rect(700, 400, 800, 600, 0x80ff80ff, VGA_ALPHA_BLEND);

  while (1) {
    char sc = 0;
    while (sc == 0) sc = kbd_getKeyEvent().scancode;

    char buf[] = "Pressed: X";
    buf[9] = sc;
    vga_gradient(10, 10, 110, 36, 0x0020a0, 0x2040c0, VGA_GRAD_V);
    vga_frame(10, 10, 110, 36, 0xffffff, 0);
    vga_text(14, 18, buf, 0xffffff, 0, 0);
  }

  return 0;
}
