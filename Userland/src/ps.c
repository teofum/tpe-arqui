#include <print.h>
#include <process.h>
#include <ps.h>
#include <shell.h>

static char *const state_str[] = {
  [PROC_STATE_RUNNING] = COL_GREEN "Running" COL_RESET,
  [PROC_STATE_BLOCKED] = COL_YELLOW "Blocked" COL_RESET,
  [PROC_STATE_EXITED] = COL_GRAY "Exited " COL_RESET,
};

int ps() {
  printf(
    COL_GRAY
    "PID  Name             State    Priority Foreground RSP               \n"
  );
  printf(
    COL_GRAY
    "-----------------------------------------------------------------------\n"
  );

  proc_info_t info;
  for (pid_t i = 0; i <= MAX_PID; i++) {
    if (proc_info(i, &info)) {
      printf(
        COL_MAGENTA "%4u " COL_RESET "%16s %s " COL_MAGENTA "%9u " COL_BLUE
                    "%10s " COL_MAGENTA "%#016llx\n",
        info.pid, info.description, state_str[info.state], info.priority,
        info.foreground ? "Yes" : " ", info.rsp
      );
    }
  }

  return 0;
}
