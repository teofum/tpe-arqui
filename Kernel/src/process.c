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

void *last_iretq_frame = 0;

extern void _proc_init(proc_entrypoint_t entry_point, void *stack);
extern void _proc_timer_interrupt();

static pid_t get_first_unused_pid() {
  pid_t pid = 0;
  while (proc_control_table[pid].stack != NULL) pid++;

  return pid;
}

void proc_spawn(proc_entrypoint_t entry_point, pid_t *new_pid) {
  *new_pid = get_first_unused_pid();

  proc_control_block_t *pcb = &proc_control_table[*new_pid];

  pcb->stack = mem_alloc(STACK_SIZE);
  pcb->rsp = (uint64_t) pcb->stack + STACK_SIZE;

  // Initialize process stack
  uint64_t *process_stack = (uint64_t *) pcb->rsp;
  process_stack -= 5;
  process_stack[4] = 0x0;                   // SS
  process_stack[3] = pcb->rsp;              // RSP
  process_stack[2] = 0x202;                 // RFLAGS
  process_stack[1] = 0x8;                   // CS
  process_stack[0] = (uint64_t) entry_point;// RIP

  process_stack -= 14;
  process_stack[13] = 0;// RAX
  process_stack[12] = 0;// RBX
  process_stack[11] = 0;// RCX
  process_stack[10] = 0;// RDX
  process_stack[9] = 0; // RSI
  process_stack[8] = 0; // RDI
  process_stack[7] = 0; // R8
  process_stack[6] = 0; // R9
  process_stack[5] = 0; // R10
  process_stack[4] = 0; // R11
  process_stack[3] = 0; // R12
  process_stack[2] = 0; // R13
  process_stack[1] = 0; // R14
  process_stack[0] = 0; // R15

  pcb->rsp = (uint64_t) process_stack;

  scheduler_enqueue(*new_pid);
  scheduler_enqueue(proc_running_pid);

  scheduler_force_next = 1;
  _proc_timer_interrupt();
}

void proc_exit(int return_code) {
  proc_control_block_t *pcb = &proc_control_table[proc_running_pid];

  // TODO: unblock waiting process with return code...

  // Clean up PCB for the current process
  mem_free(pcb->stack);
  pcb->stack = NULL;

  scheduler_force_next = 1;
  _proc_timer_interrupt();
}

void proc_init(proc_entrypoint_t entry_point) {
  pid_t new_pid = get_first_unused_pid();

  proc_control_block_t *pcb = &proc_control_table[new_pid];

  pcb->stack = mem_alloc(STACK_SIZE);
  pcb->rsp = (uint64_t) pcb->stack + STACK_SIZE;

  proc_running_pid = new_pid;
  _proc_init(entry_point, (void *) pcb->rsp);
}
