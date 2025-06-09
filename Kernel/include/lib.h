#ifndef LIB_H
#define LIB_H

#include <stdint.h>

char *cpuVendor(char *result);
void _hlt(void);
void _cli(void);
void _sti(void);
void haltcpu(void);

#endif
