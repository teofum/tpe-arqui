#include <print.h>
#include <stddef.h>
#include <syscall.h>

int utostr(
  char *buf, uint64_t n, uint8_t base, uint8_t minLength, char padding
) {
  uint8_t digits[64];
  uint8_t ndigits = 0;
  int32_t len = 0;

  do {
    digits[ndigits++] = n % base;
    n /= base;
  } while (n > 0);

  for (int i = 0; i < minLength - ndigits; i++) buf[len++] = padding;

  for (int i = 0; i < ndigits; i++) {
    uint8_t digit = digits[ndigits - 1 - i];
    buf[len++] = digit >= 10 ? 'A' + (digit - 10) : '0' + digit;
  }

  buf[len] = 0;
  return len;
}

static int32_t vsprintf(char *buf, const char *fmt, va_list args) {
  int32_t len = 0;

  char t_char = 0;
  int64_t t_i64 = 0;
  uint64_t t_u64 = 0;
  const char *t_str = NULL;
  uint8_t base = 10;

  static char buffer[256];

  char c;
  while ((c = *fmt++)) {
    if (c == '%') {
      int done = 0;
      char padding = ' ';
      uint8_t minLength = 0;
      int printBase = 0, islong = 0;

      while (!done) {
        c = *fmt++;

        if (c == '#') {
          printBase = 1;
          c = *fmt++;
        }

        // Switch number padding mode on leazing zero, discard any excess zeros
        while (c == '0') {
          padding = '0';
          c = *fmt++;
        }

        // Parse digits as minimum length
        while (c >= '0' && c <= '9') {
          minLength *= 10;
          minLength += c - '0';
          c = *fmt++;
        }

        switch (c) {
          case 'l':
            islong = 1;
            break;
          case '%':
            buf[len++] = '%';
            done = 1;
            break;
          case 'c':
            t_char = va_arg(args, int);
            buf[len++] = t_char;
            done = 1;
            break;
          case 's':
            t_str = va_arg(args, const char *);
            while ((c = *t_str++) != 0) buf[len++] = c;
            done = 1;
            break;
          case 'd':
            t_i64 = islong ? va_arg(args, int64_t) : va_arg(args, int32_t);
            done = 1;
            // TODO: signed integer support
            break;
          case 'u':
          case 'x':
          case 'b':
            if (printBase && (c == 'x' || c == 'b')) {
              buf[len++] = '0';
              buf[len++] = c;
            }

            base = (c == 'x') ? 16 : (c == 'b') ? 2 : 10;
            t_u64 = islong ? va_arg(args, uint64_t) : va_arg(args, uint32_t);
            t_u64 = utostr(buffer, t_u64, base, minLength, padding);
            for (int i = 0; i < t_u64; i++) buf[len++] = buffer[i];
            done = 1;
            break;
        }
      }
    } else {
      buf[len++] = c;
    }
  }

  buf[len] = 0;
  return len;
}

int32_t sprintf(char *buf, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int32_t len = vsprintf(buf, fmt, args);

  va_end(args);
  return len;
}

int32_t printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  static char buf[512];
  int32_t len = vsprintf(buf, fmt, args);
  len = _syscall(SYS_WRITE, buf, len);

  va_end(args);
  return len;
}
