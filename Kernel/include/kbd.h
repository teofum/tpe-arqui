#ifndef KBD_H
#define KBD_H
#include <stdint.h>

extern uint8_t _kbd_read();

#define BUFFER_SIZE 64
struct buffer{
char* data[BUFFER_SIZE];
int write_pos;
uint8_t isChar; //flag
};

struct buffer _kbd_readKeyCombo();

#endif