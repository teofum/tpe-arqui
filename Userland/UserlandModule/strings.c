#include <strings.h>

int strcmp(const char *a, const char *b) {
  while (*a == *b) {
    if (*a == 0) return 0;
    a++;
    b++;
  }

  return *a - *b;
}

const char *strsplit(char *out, const char *str, char delim) {
  while (*str != 0 && *str != delim) { *out++ = *str++; }

  *out = 0;
  return *str == 0 ? NULL : str + 1;
}
