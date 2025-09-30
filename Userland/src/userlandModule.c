#include <io.h>
#include <kbd.h>
#include <print.h>
#include <process.h>
#include <shell.h>
#include <sst.h>
#include <stdint.h>
#include <stdio.h>

#define SST

char *v = (char *) 0xB8000 + 79 * 2;

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


int test_b(uint64_t argc, const char **argv) {
  printf(COL_MAGENTA "%s my name is %s\n", argv[1], argv[0]);

  for (int i = 0; i < 300; i++) { writes(COL_GREEN "B"); }

  return 42;
}

int main() {
  writes(COL_BLUE "Spawning B\n");

  const char *argv[] = {"test_b", "Hello world!"};
  pid_t child_pid = proc_spawn(test_b, sizeof(argv) / sizeof(argv[0]), argv);

  printf("\nspawned process with pid %u\n", (uint32_t) child_pid);
  for (int i = 0; i < 50; i++) { writes(COL_RED "A"); }
  printf("\nwaiting for process with pid %u\n", (uint32_t) child_pid);

  int ret = proc_wait(child_pid);
  printf(
    "\nprocess with pid %u exited with code %u\n", (uint32_t) child_pid, ret
  );

  while (1) { writes(COL_RED "A"); }
}

// int main() {
// #ifdef SST
//   int test_result = sst_run_tests();
//
//   if (test_result) {
//     printf(
//       "[" COL_RED "SST FAIL" COL_RESET "] " COL_RED
//       "%u test failures\n" COL_RESET,
//       test_result
//     );
//   } else {
//     writes("[" COL_GREEN "SST OK" COL_RESET "] All tests passed!\n");
//   }
//
//   writes("Press any key to continue\n");
//
//   int key = 0;
//   while (!key) key = kbd_get_key_event().key;
// #endif
//
//   io_clear();
//   writes(mascot);
//   writes("\n\n\x1A R;Welcome to \x1A 195,248,132;carpinchOS\x1A R;!\n");
//
//   start_shell();
//
//   return 0xDEADBEEF;
// }
