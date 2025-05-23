#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <libasm.h>
#include <stdint.h>

void loadIDT();

void initInterrupts();

void initSyscalls();

#endif /* INTERRUPTS_H */
