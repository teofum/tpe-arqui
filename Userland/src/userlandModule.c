#include <io.h>
#include <kbd.h>
#include <print.h>
#include <process.h>
#include <shell.h>
#include <sst.h>
#include <stdint.h>
#include <stdio.h>

// TODO move this somewhere else
#define lengthof(x) (sizeof((x)) / sizeof((x)[0]))

#define SST

const char *mascot =
  "\x1A 187,111,68;"
  "                     .%-%@.                                         \n"
  "                  .**++##=@*                                        \n"
  "              .=%.    .%=%%.*-.                                     \n"
  "            =*-   .::   .=    -+*:                                  \n"
  "         .+*.    :.:               -%%+:::..                        \n"
  "        ==+==                             .-===**-                  \n"
  "        *:  *:                                     -%-.             \n"
  "        .#  :*            #       *-                  .=*.          \n"
  "          %:=.%.     .:=%.        @+              .+=     %*.       \n"
  "          .-+*#*====*:          -=-               -*.      .#=      \n"
  "                      -*.                                   +=#     \n"
  "                        -*                                   -+     \n"
  "                          -*.              ..-%%%:            .*.   \n"
  "                           :*.            *=.                  :+.  \n"
  "                             #.        .*=                      *:  \n"
  "                              %.       ::                        %  \n"
  "                              :*.                         .      .* \n"
  "                              :#-*+.                    =*%       #.\n"
  "                              :*   =+.                .=*.        -*\n"
  "                              .+.  =%-        +                  %. \n"
  "                              :*  .-:#.    .:%#                 .%  \n"
  "                          .+***-  :* -@+.                      -+.  \n"
  "                          #@*=.  .%=#:. +%..                 .-+    \n"
  "                           :++-==. =#*+#**#@#=.           .+*=      \n"
  "                                                  .-#%%#-.\n";

void test_a() {
  for (int i = 0; i < 100; i++) { writes(COL_RED "A"); }
  proc_exit(0);
}
void test_b() {
  for (int i = 0; i < 100; i++) { writes(COL_GREEN "B"); }
  proc_exit(0);
}
void test_c() {
  for (int i = 0; i < 100; i++) { writes(COL_BLUE "C"); }
  proc_exit(0);
}
void test_d() {
  for (int i = 0; i < 100; i++) { writes(COL_YELLOW "D"); }
  proc_exit(0);
}
void test_e() {
  for (int i = 0; i < 100; i++) { writes(COL_MAGENTA "E"); }
  proc_exit(0);
}

void main() {
  writes(COL_BLUE "Spawning A\n");
  pid_t test_a_pid = proc_spawn(test_a, 0, NULL, 0);
  writes(COL_BLUE "Spawning B\n");
  pid_t test_b_pid = proc_spawn(test_b, 0, NULL, 1);
  writes(COL_BLUE "Spawning C\n");
  pid_t test_c_pid = proc_spawn(test_c, 0, NULL, 2);
  writes(COL_BLUE "Spawning D\n");
  pid_t test_d_pid = proc_spawn(test_d, 0, NULL, 3);
  writes(COL_BLUE "Spawning E\n");
  pid_t test_e_pid = proc_spawn(test_e, 0, NULL, 4);

  proc_wait(test_e_pid);

  writes(COL_BLUE "end");
  while (1) {};//writes(COL_GRAY "_(:v \\)/_"); }
}

/*
int main() {
#ifdef SST
  int test_result = sst_run_tests();
  if (test_result) {
    printf(
      "[" COL_RED "SST FAIL" COL_RESET "] " COL_RED
      "%u test failures\n" COL_RESET,
      test_result
    );
  } else {
    writes("[" COL_GREEN "SST OK" COL_RESET "] All tests passed!\n");
  }

  writes("Press any key to continue\n");

  kbd_get_key_event();
#endif

  io_clear();
  writes(mascot);
  writes("\n\n");

  char *shell_args[] = {"cash"};
  pid_t shell_pid =
    proc_spawn(cash, lengthof(shell_args), shell_args, DEFAULT_PRIORITY);
  proc_wait(shell_pid);

  return 0xDEADBEEF;
}
*/