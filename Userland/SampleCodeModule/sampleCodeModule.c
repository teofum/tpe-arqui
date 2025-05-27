/* sampleCodeModule.c */

#include <syscall.h>

char *v = (char *) 0xB8000 + 79 * 2;

static int var1 = 0;
static int var2 = 5;

int main() {
  // syscall test
  _syscall(
    SYS_WRITES,
    "This is a string written from userland module with syscall write!\n"
  );

  //Test if BSS is properly set up
  if (var1 == 0 && var2 == 0) return 0xDEADC0DE;

  return 0xDEADBEEF;
}
