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


extern size_t _getrsp();
void test_b() {
  printf("my rsp is at %#08lx\n", _getrsp());
  for (int i = 0; 1; i++) { writes(COL_GREEN "B"); }
  proc_exit(0);
}

void main() {
  writes("hello world\n");
  writes(COL_BLUE "Spawning B\n");
  printf("before vars %#08lx\n", _getrsp());
  int test = 42;
  printf("test variable at %#08lx\n", (size_t) &test);
  pid_t pid = 99, pid2 = 98, pid3 = 97;
  printf("before spawn %#08lx\n", _getrsp());
  printf("test variable at %#08lx\n", (size_t) &test);
  proc_spawn(test_b, &pid);
  printf("after spawn %#08lx\n", _getrsp());
  printf("test variable at %#08lx\n", (size_t) &test);
  printf("spawned process with pid %u\n", (uint32_t) pid);
  while (1) {
    // writes(COL_RED "A");
    printf(" %u", test++);
  }
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
