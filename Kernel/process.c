#include <mem.h>
#include <process.h>

#define STACK_SIZE (1024 * 64)// Give each process 64k stack

proc_control_block_t proc_control_table[MAX_PID + 1] = {0};

pid_t proc_running_pid = 0;

void *proc_kernel_stack = NULL;

void *last_iretq_frame = 0;

proc_registers_t *proc_get_registers_addr_for_current_process() {
  proc_control_block_t *pcb = &proc_control_table[proc_running_pid];
  return &pcb->registers;
}

static pid_t get_first_unused_pid() {
  pid_t pid = 0;
  while (proc_control_table[pid].stack != NULL) pid++;

  return pid;
}

extern void _proc_jump_to_spawned(proc_entrypoint_t entry_point, void *stack);

void proc_spawn(proc_entrypoint_t entry_point) {
  pid_t new_pid = get_first_unused_pid();

  proc_control_block_t *pcb = &proc_control_table[new_pid];

  void *stack_begin = mem_alloc(STACK_SIZE);
  pcb->stack = stack_begin + STACK_SIZE - 8;

  proc_running_pid = new_pid;
  _proc_jump_to_spawned(entry_point, pcb->stack);
}

extern void _proc_init(proc_entrypoint_t entry_point, void *stack);

void proc_init(proc_entrypoint_t entry_point) {
  pid_t new_pid = get_first_unused_pid();

  proc_control_block_t *pcb = &proc_control_table[new_pid];

  void *stack_begin = mem_alloc(STACK_SIZE);
  pcb->stack = stack_begin + STACK_SIZE - 8;

  proc_running_pid = new_pid;
  _proc_init(entry_point, pcb->stack);
}
