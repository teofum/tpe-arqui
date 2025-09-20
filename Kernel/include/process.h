#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Support up to 4096 processes
 */
#define MAX_PID 0xfff

typedef int16_t pid_t;
typedef int (*proc_entrypoint_t)();

typedef struct {
  uint64_t rax, rbx, rcx, rdx, rsi, rdi;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;

  // Interrupt vector stuff
  uint64_t rip, cs, rflags, rsp, ss;

  uint64_t rbp, ds, es, fs, gs;
} proc_registers_t;

typedef struct {
  void *stack;
  size_t stack_size;

  proc_registers_t registers;
} proc_control_block_t;

extern proc_control_block_t proc_control_table[];

extern pid_t proc_running_pid;

extern void *proc_kernel_stack;

proc_registers_t *proc_get_registers_addr_for_current_process();

void proc_spawn(void *entry_point);

void proc_kill(pid_t pid);

void proc_switch(pid_t pid);

/*
 * 1.  Some process A is running
 * 2.  Interrupt (timer)
 * 3.  Pop the interrupt return data (rsp, rip, cs, ss, rflags) from the stack
 * 4.  Store it + register state in the current process PCB
 * 5.  Set rsp to kernel/scheduler stack
 * 6.  Call the timer interrupt handler
 * 7.  Potentially run scheduler -> changes current process
 * 8.  Get interrupt return data + register state from current process PCB (might have changed!)
 * 9.  Restore registers
 * 10. Push interrupt return data to stack
 * 11. iretq -> will return to the now running process!!
 */

#endif
