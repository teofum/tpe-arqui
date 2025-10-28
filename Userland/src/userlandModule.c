#include "fd.h"
#include <io.h>
#include <kbd.h>
#include <print.h>
#include <process.h>
#include <scheduler.h>
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
    writes(STDOUT, "[" COL_GREEN "SST OK" COL_RESET "] All tests passed!\n");
  }

  writes(STDOUT, "Press any key to continue\n");

  kbd_get_key_event();
#endif

  io_clear();
  writes(STDOUT, mascot);
  writes(STDOUT, "\n\n");

  char *shell_args[] = {"cash"};
  pid_t shell_pid = proc_spawn(cash, lengthof(shell_args), shell_args, NULL);
  proc_wait(shell_pid);

  return 0xDEADBEEF;
}
