#include <lib.h>
#include <mem.h>
#include <process.h>
#include <scheduler.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define STACK_SIZE (1024 * 64)// Give each process 64k stack

proc_control_block_t proc_control_table[MAX_PID + 1] = {0};

pid_t proc_running_pid = 0;

void *proc_kernel_stack = NULL;

void *last_iretq_frame = 0;

extern void _proc_jump_to_spawned(
  proc_entrypoint_t entry_point, void *stack, proc_registers_t *spawner_regs
);
extern void _proc_jump_to_next();
extern void _proc_use_kernel_stack();
extern void _proc_init(proc_entrypoint_t entry_point, void *stack);

static pid_t get_first_unused_pid() {
  pid_t pid = 0;
  while (proc_control_table[pid].stack != NULL) pid++;

  return pid;
}

proc_registers_t *proc_get_registers_addr_for_current_process() {
  proc_control_block_t *pcb = &proc_control_table[proc_running_pid];
  return &pcb->registers;
}

void *proc_get_kernel_stack() {
  proc_control_block_t *pcb = &proc_control_table[proc_running_pid];

  return (uint8_t *) pcb->kernel_stack + STACK_SIZE - 8;
}

extern size_t _getrsp();
void proc_spawn(proc_entrypoint_t entry_point, pid_t *new_pid) {
  *new_pid = get_first_unused_pid();

  _cli();
  proc_control_block_t *pcb = &proc_control_table[*new_pid];

  pcb->mode = PROC_MODE_USER;
  pcb->stack = mem_alloc(STACK_SIZE);
  pcb->kernel_stack = mem_alloc(STACK_SIZE);
  uint8_t *stack_begin = (uint8_t *) pcb->stack + STACK_SIZE - 8;

  proc_registers_t *spawner_regs =
    proc_get_registers_addr_for_current_process();

  scheduler_enqueue(proc_running_pid);
  proc_running_pid = *new_pid;
  _proc_jump_to_spawned(entry_point, stack_begin, spawner_regs);

  // Unreachable! _proc_jump_to_spawned actually sets the value of rax to
  // the spawned process PID, which has the same effect
  // return new_pid;
}

void proc_exit(int return_code) {
  proc_control_block_t *pcb = &proc_control_table[proc_running_pid];

  _cli();// Critical region

  _proc_use_kernel_stack();

  // Clean up PCB for the current process
  mem_free(pcb->stack);
  mem_free(pcb->kernel_stack);
  pcb->stack = NULL;
  pcb->kernel_stack = NULL;

  // TODO: unblock waiting process with return code...

  // Call the scheduler and move on to next process
  scheduler_next();
  _sti();
  _proc_jump_to_next();
}

void proc_init(proc_entrypoint_t entry_point) {
  pid_t new_pid = get_first_unused_pid();

  proc_control_block_t *pcb = &proc_control_table[new_pid];

  pcb->stack = mem_alloc(STACK_SIZE);
  pcb->kernel_stack = mem_alloc(STACK_SIZE);
  uint8_t *stack_begin = (uint8_t *) pcb->stack + STACK_SIZE - 8;

  proc_running_pid = new_pid;
  _proc_init(entry_point, stack_begin);
}
