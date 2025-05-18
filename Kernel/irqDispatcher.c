#include <irqDispatcher.h>

void (*irqHandlers[MAX_INTERRUPTS])();

void irqDispatcher(uint64_t irq) {
  if (irq < MAX_INTERRUPTS)
    irqHandlers[irq]();
}

void setInterruptHandler(uint64_t irq, void (*handler)()) {
  if (irq < MAX_INTERRUPTS)
    irqHandlers[irq] = handler;
}

