#ifndef KBD_H
#define KBD_H
#include <stdint.h>

#define BUFFER_SIZE    5

struct buffer{
char* data[BUFFER_SIZE];
int write_pos;
uint8_t isChar; //flag
};

extern uint8_t _kbd_read();
extern uint8_t _kbd_readState();

struct buffer _kbd_readKeyCombo();



#endif