#include "shell.h"
#include <syscall.h>

char *v = (char *) 0xB8000 + 79 * 2;

static int var1 = 0;
static int var2 = 5;

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
  _syscall(SYS_CLEAR);
  _syscall(SYS_WRITES, mascot);
  _syscall(
    SYS_WRITES, "\n\n\x1A R;Welcome to \x1A 195,248,132;carpinchOS\x1A R;!\n"
  );
  startShell();

  return 0xDEADBEEF;
}
