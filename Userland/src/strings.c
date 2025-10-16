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

const uint32_t
strsplit(char **out_string_starts, char *out, const char *str, char delim) {
  int i = 0, j = 0;
  out_string_starts[0] = out;
  for (; str[i] != 0; i++) {
    if (str[i] == ' ') {
      out[i] = 0;
      out_string_starts[++j] = &out[i + 1];
    } else {
      out[i] = str[i];
    }
  }
  out[i] = 0;

  return j;
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

int memcpy(char *dst, const char *src, size_t len) {
  for (size_t i = 0; i < len; i++) { *dst++ = *src++; }

  return 0;
}
