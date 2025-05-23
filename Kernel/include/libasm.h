#ifndef LIBASM_H
#define LIBASM_H

#include <stdint.h>

/* Funciones de CPU */
char *cpuVendor(char *vendor);
void _hlt(void);
void _cli(void);
void _sti(void);
void haltcpu(void);

/* Funciones para PIC */
void picMasterMask(uint8_t mask);
void picSlaveMask(uint8_t mask);

/* Handlers de interrupciones *///esto no se si borrarlo, lo pase a interrupts.h
// void _irq00Handler(void);
// void _irq01Handler(void);
// void _irq80Handler(void);
// void _exception0Handler(void);

#endif /* LIBASM_H */
