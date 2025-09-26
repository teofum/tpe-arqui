#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Support up to 4096 processes
 */
#define MAX_PID 0xfff

typedef int16_t pid_t;
typedef void (*proc_entrypoint_t)();

typedef enum { PROC_MODE_USER = 0, PROC_MODE_KERNEL } proc_mode_t;

typedef struct {
  uint64_t rax, rbx, rcx, rdx, rsi, rdi;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;

  // Interrupt vector stuff
  uint64_t rip, cs, rflags, rsp, ss;

  uint64_t rbp, ds, es, fs, gs;
} proc_registers_t;

typedef struct {
  void *stack;
  void *kernel_stack;
  proc_mode_t mode;

  proc_registers_t registers;
} proc_control_block_t;

extern proc_control_block_t proc_control_table[];

extern pid_t proc_running_pid;

extern void *proc_kernel_stack;

/*
 * Kernel-only function. Spawns a process from kernel, without the usual plumbing
 * to keep the calling process running. Used to start the first "init" process.
 */
void proc_init(proc_entrypoint_t entry_point);

/*
 * Spawn a process. Returns the PID of the new process.
 */
void proc_spawn(proc_entrypoint_t entry_point, pid_t *new_pid);

/*
 * Terminate the current process.
 */
void proc_exit(int return_code);

/*
 * Wait for a process to end. Blocks the caller until the waited process
 * terminates.
 */
void proc_wait(pid_t pid);

/*
 * Kill a running process.
 */
void proc_kill(pid_t pid);

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
