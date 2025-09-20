#include <process.h>

proc_control_block_t proc_control_table[MAX_PID + 1] = {0};

pid_t proc_running_pid = 0;

void *proc_kernel_stack = NULL;

proc_registers_t *proc_get_registers_addr_for_current_process() {
  return &proc_control_table[proc_running_pid].registers;
}
