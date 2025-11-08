#include <utils.h>

uint32_t parse_uint(const char *s) {
  uint32_t r = 0;
  while (*s >= '0' && *s <= '9') {
    r *= 10;
    r += *s - '0';
    s++;
  }
  return r;
}
