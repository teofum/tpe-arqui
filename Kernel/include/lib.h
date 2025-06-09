#ifndef LIB_H
#define LIB_H

#include <stdint.h>

extern char *cpuVendor(char *result);
extern void _hlt();
extern void _cli();
extern void _sti();
extern void haltcpu();

/*
 * Lee un byte de un puerto de E/S y devuelve valor leído del puerto
 */
extern uint8_t inb(uint16_t port);

/*
 * Escribe un byte en un puerto de E/S
 */
extern void outb(uint16_t port, uint8_t value);

#endif
