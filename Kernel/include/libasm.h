#ifndef LIBASM_H
#define LIBASM_H

#include <stdint.h>

/* Funciones de CPU */
char *cpuVendor(char *vendor);
void _hlt(void);
void _cli(void);
void _sti(void);
void haltcpu(void);

#endif /* LIBASM_H */
