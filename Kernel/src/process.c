#include <lib.h>
#include <mem.h>
#include <pqueue.h>
#include <process.h>
#include <scheduler.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>
#include <types.h>

#define STACK_SIZE (1024 * 64)// Give each process 64k stack

proc_control_block_t proc_control_table[MAX_PID + 1] = {0};

pid_t proc_running_pid = 0;
pid_t proc_foreground_pid = 0;

void *last_iretq_frame = 0;

extern void _proc_init(proc_entrypoint_t entry_point, void *stack);
extern void _proc_timer_interrupt();

static pid_t get_first_unused_pid() {
  pid_t pid = 0;
  while (proc_control_table[pid].stack != NULL) pid++;

  return pid;
}

static void
proc_start(proc_entrypoint_t entry_point, uint64_t argc, char *const *argv) {
  int ret = entry_point(argc, argv);
  proc_exit(ret);
}

static void proc_initialize_process(
  pid_t pid, proc_entrypoint_t entry_point, uint64_t argc, char *const *argv,
  priority_t priority
) {
  proc_control_block_t *pcb = &proc_control_table[pid];

  char **argv_copy = mem_alloc(argc * sizeof(char *));
  for (int i = 0; i < argc; i++) {
    argv_copy[i] = mem_alloc(strlen(argv[i]));
    strcpy(argv_copy[i], argv[i]);
  }

  pcb->argc = argc;
  pcb->argv = argv_copy;
  pcb->description = argv_copy[0];
  pcb->stack = mem_alloc(STACK_SIZE);
  pcb->rsp = (uint64_t) pcb->stack + STACK_SIZE;
  pcb->state = PROC_STATE_RUNNING;
  pcb->waiting_processes = pqueue_create();
  pcb->n_waiting_processes = 0;
  pcb->priority = priority;

  // Initialize process stack
  uint64_t *process_stack = (uint64_t *) pcb->rsp;
  process_stack -= 5;
  process_stack[4] = 0x0;                  // SS
  process_stack[3] = pcb->rsp;             // RSP
  process_stack[2] = 0x202;                // RFLAGS
  process_stack[1] = 0x8;                  // CS
  process_stack[0] = (uint64_t) proc_start;// RIP

  process_stack -= 15;
  process_stack[14] = 0;                    // RBP
  process_stack[13] = 0;                    // RAX
  process_stack[12] = 0;                    // RBX
  process_stack[11] = 0;                    // RCX
  process_stack[10] = (uint64_t) argv_copy; // RDX
  process_stack[9] = argc;                  // RSI
  process_stack[8] = (uint64_t) entry_point;// RDI
  process_stack[7] = 0;                     // R8
  process_stack[6] = 0;                     // R9
  process_stack[5] = 0;                     // R10
  process_stack[4] = 0;                     // R11
  process_stack[3] = 0;                     // R12
  process_stack[2] = 0;                     // R13
  process_stack[1] = 0;                     // R14
  process_stack[0] = 0;                     // R15

  pcb->rsp = (uint64_t) process_stack;
}

static int proc_idle() {
  while (1) {
    scheduler_force_next = 1;
    _hlt();
  }

  // Never returns
}

void proc_init(proc_entrypoint_t entry_point) {
  /*
   * Initialize the idle process, but don't run it
   */
  char *const idle_argv[1] = {"idle"};
  proc_initialize_process(IDLE_PID, proc_idle, 1, idle_argv, MAX_PRIORITY + 1);
  //invalid priority pq no deberia entrar en la cola

  /*
   * Initialize and start the init process. We "bootstrap" the process
   * system by jumping to this first process directly.
   */
  pid_t new_pid = get_first_unused_pid();

  proc_control_block_t *pcb = &proc_control_table[new_pid];

  pcb->argv = NULL;
  pcb->description = "init";
  pcb->stack = mem_alloc(STACK_SIZE);
  pcb->rsp = (uint64_t) pcb->stack + STACK_SIZE;
  pcb->state = PROC_STATE_RUNNING;
  pcb->waiting_processes = pqueue_create();
  pcb->n_waiting_processes = 0;

  proc_running_pid = new_pid;
  proc_foreground_pid = new_pid;
  _proc_init(entry_point, (void *) pcb->rsp);
}

void proc_yield() {
  scheduler_force_next = 1;
  _proc_timer_interrupt();
}

void proc_block() {
  proc_control_block_t *pcb = &proc_control_table[proc_running_pid];
  pcb->state = PROC_STATE_BLOCKED;

  proc_yield();
}

void proc_blockpid(pid_t pid) {
  proc_control_block_t *pcb = &proc_control_table[pid];
  pcb->state = PROC_STATE_BLOCKED;

  // TODO remove from scheduler
}

void proc_runpid(pid_t pid) {
  proc_control_block_t *pcb = &proc_control_table[pid];
  if (pcb->state == PROC_STATE_BLOCKED) {
    pcb->state = PROC_STATE_RUNNING;
    scheduler_enqueue(pid);
  }
}

pid_t proc_spawn(
  proc_entrypoint_t entry_point, uint64_t argc, char *const *argv,
  priority_t priority
) {
  pid_t new_pid = get_first_unused_pid();

  proc_initialize_process(new_pid, entry_point, argc, argv, priority);
  scheduler_enqueue(new_pid);
  proc_yield();

  return new_pid;
}

void proc_exit(int return_code) {
  proc_control_block_t *pcb = &proc_control_table[proc_running_pid];

  pcb->state = PROC_STATE_EXITED;
  pcb->return_code = return_code;

  while (!pqueue_empty(pcb->waiting_processes)) {
    pid_t waiting_pid = pqueue_dequeue(pcb->waiting_processes);

    proc_control_block_t *waiting_pcb = &proc_control_table[waiting_pid];
    waiting_pcb->state = PROC_STATE_RUNNING;

    scheduler_enqueue(waiting_pid);
  }

  proc_yield();
}

static void proc_destroy(pid_t pid) {
  proc_control_block_t *pcb = &proc_control_table[pid];

  mem_free(pcb->stack);
  for (int i = 0; i < pcb->argc; i++) mem_free(pcb->argv[0]);
  mem_free((void *) pcb->argv);
  pcb->argc = 0;
  pcb->argv = NULL;
  pcb->description = NULL;
  pcb->stack = NULL;
  pqueue_destroy(pcb->waiting_processes);
}

static void proc_make_foreground(pid_t pid) {
  proc_foreground_pid = pid;

  proc_control_block_t *pcb = &proc_control_table[pid];
  if (pcb->waiting_for_foreground && pcb->state == PROC_STATE_BLOCKED) {
    pcb->waiting_for_foreground = 0;
    pcb->state = PROC_STATE_RUNNING;
    scheduler_enqueue(pid);
  }
}

int proc_wait(pid_t pid) {
  proc_control_block_t *waiting_pcb = &proc_control_table[pid];

  if (proc_foreground_pid == proc_running_pid) proc_make_foreground(pid);

  waiting_pcb->n_waiting_processes++;
  while (waiting_pcb->state != PROC_STATE_EXITED) {
    pqueue_enqueue(waiting_pcb->waiting_processes, proc_running_pid);
    proc_block();
  }

  if (proc_foreground_pid == pid) proc_make_foreground(proc_running_pid);

  int return_code = waiting_pcb->return_code;
  waiting_pcb->n_waiting_processes--;

  if (waiting_pcb->n_waiting_processes == 0) proc_destroy(pid);

  return return_code;
}

void proc_kill(pid_t pid) {
  proc_control_block_t *pcb = &proc_control_table[pid];

  pcb->state = PROC_STATE_EXITED;
  pcb->return_code = RETURN_KILLED;

  while (!pqueue_empty(pcb->waiting_processes)) {
    pid_t waiting_pid = pqueue_dequeue(pcb->waiting_processes);

    proc_control_block_t *waiting_pcb = &proc_control_table[waiting_pid];
    waiting_pcb->state = PROC_STATE_RUNNING;

    scheduler_enqueue(waiting_pid);
  }
}

pid_t proc_getpid() { return proc_running_pid; }

void proc_wait_for_foreground() {
  while (proc_foreground_pid != proc_running_pid) {
    proc_control_block_t *pcb = &proc_control_table[proc_running_pid];
    pcb->waiting_for_foreground = 1;

    proc_block();
  }
}

int proc_info(pid_t pid, proc_info_t *out_info) {
  proc_control_block_t *pcb = &proc_control_table[pid];
  if (pcb->stack == NULL) return 0;

  out_info->pid = pid;
  out_info->description = pcb->description;
  out_info->state = pcb->state;
  out_info->priority = pcb->priority;
  out_info->rsp = pcb->rsp;
  out_info->foreground = pid == proc_foreground_pid;

  return 1;
}
