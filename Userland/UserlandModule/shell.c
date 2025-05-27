#include <shell.h>
#include <syscall.h>

static void writePrompt() { _syscall(SYS_WRITES, "> "); }

static void readCommand(char *buf) {
  int inputEnd = 0;
  while (!inputEnd) {
    int len = _syscall(SYS_READ, buf);
    for (int i = 0; i < len; i++) {
      if (buf[i] == '\n') {
        len = i;
        inputEnd = 1;
      }
    }

    _syscall(SYS_WRITE, buf, len);
    buf += len;
  }

  _syscall(SYS_PUTC, '\n');
}

static int runCommand(const char *cmd) {
  if (cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'i' && cmd[3] == 't')
    return -1;
  return 0;
}

int startShell() {
  char cmdBuf[256];

  int exit = 0;
  while (!exit) {
    writePrompt();
    readCommand(cmdBuf);
    int returnCode = runCommand(cmdBuf);

    if (returnCode == -1) exit = 1;
  }

  return 0;
}
