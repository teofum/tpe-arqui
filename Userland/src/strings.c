#include <mem.h>
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

int memcpy(char *dst, const char *src, size_t len) {
  for (size_t i = 0; i < len; i++) { *dst++ = *src++; }

  return 0;
}

char *strtrim(char *str, char delim) {
  // trim start
  while (*str == delim) str++;
  char *start = str;

  // trim end
  char *end = str;
  while (*str != 0) {
    if (*str != delim) end = str;
    str++;
  }

  end++;
  *end = 0;

  return start;
}

split_result_t strsplit(const char *str, char delim) {
  uint64_t strings_count = 1;
  size_t srt_len = 0;
  for (; str[srt_len] != 0; srt_len++) {
    if (str[srt_len] == delim) strings_count++;
  }

  size_t strings_size = strings_count * sizeof(char *);
  void *out_mem = mem_alloc(strings_size + srt_len);

  char **strings = out_mem;
  char *buf = (char *) out_mem + strings_size;

  int i = 0, j = 0;
  strings[0] = buf;
  for (; str[i] != 0; i++) {
    if (str[i] == delim) {
      buf[i] = 0;
      strings[++j] = &buf[i + 1];
    } else {
      buf[i] = str[i];
    }
  }
  buf[i] = 0;

  return (split_result_t) {
    .count = strings_count,
    .strings = strings,
  };
}
