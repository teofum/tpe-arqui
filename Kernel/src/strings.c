#include <strings.h>

int strcmp(const char *a, const char *b) {
  while (*a == *b) {
    if (*a == 0) return 0;
    a++;
    b++;
  }

  return *a - *b;
}

uint64_t strlen(const char *s) {
  uint64_t len = 0;
  while (*s++) len++;

  return len;
}

int strcpy(char *dst, const char *src) {
  int i = 0;
  while (*src != 0) {
    *dst++ = *src++;
    i++;
  }
  *dst = 0;

  return i;
}
