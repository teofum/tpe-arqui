/* sampleCodeModule.c */

#include "shell.h"
#include <golfGame.h>
#include <syscall.h>

char *v = (char *) 0xB8000 + 79 * 2;

static int var1 = 0;
static int var2 = 5;

int main() {
  _syscall(SYS_CLEAR);
  _syscall(SYS_WRITES, "Welcome to userland\n");

  gg_startGame();

  startShell();

  return 0xDEADBEEF;
}
