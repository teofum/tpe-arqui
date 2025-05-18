#ifndef IRQ_DISPATCHER_H
#define IRQ_DISPATCHER_H

#include <stdint.h>

#define MAX_INTERRUPTS 256

void irqDispatcher(uint64_t irq);
void setInterruptHandler(uint64_t irq, void (*handler)());

#endif

