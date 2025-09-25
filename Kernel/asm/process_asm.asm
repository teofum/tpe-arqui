section .text

;------------------------------------------------------------------------------
; Macros
;------------------------------------------------------------------------------

%macro pushall 0
  push rax
  push rcx
  push rdx
  push rsi
  push rdi
  push r8
  push r9
  push r10
  push r11
%endmacro

%macro popall 0
  pop r11
  pop r10
  pop r9
  pop r8
  pop rdi
  pop rsi
  pop rdx
  pop rcx
  pop rax
%endmacro

global _proc_jump_to_spawned
extern last_iretq_frame
_proc_jump_to_spawned:
  mov [rdx + 0x00], rax
  mov [rdx + 0x08], rbx
  mov [rdx + 0x10], rcx
  mov [rdx + 0x18], rdx
  mov [rdx + 0x20], rsi
  mov [rdx + 0x28], rdi

  mov [rdx + 0x30], r8
  mov [rdx + 0x38], r9
  mov [rdx + 0x40], r10
  mov [rdx + 0x48], r11
  mov [rdx + 0x50], r12
  mov [rdx + 0x58], r13
  mov [rdx + 0x60], r14
  mov [rdx + 0x68], r15

  ; Get RIP from the last iretq frame
  mov rbx, [last_iretq_frame]
  mov rbx, [rbx]
  mov [rdx + 0x70], rbx
  mov qword [rdx + 0x78], 0x8 ; CS

  pushf
  pop qword [rdx + 0x80] ; RFLAGS

  mov rbx, [last_iretq_frame]
  mov rbx, [rbx + 8 * 3]
  mov [rdx + 0x88], rbx ; RSP

  mov qword [rax + 0x90], 0x0 ; SS

  mov [rdx + 0x98], rbp
  mov [rdx + 0xA0], ds
  mov [rdx + 0xA8], es
  mov [rdx + 0xB0], fs
  mov [rdx + 0xB8], gs

  mov rbp, rsi ; Reset rbp
  mov rsp, rsi ; Move rsp to process stack
  jmp rdi ; Jump to process entry point

global _proc_jump_to_next
extern proc_get_registers_addr_for_current_process
_proc_jump_to_next:
  call proc_get_registers_addr_for_current_process

  ; Restore register and interrupt frame from PCB
  mov rbx, [rax + 0x08]
  mov rcx, [rax + 0x10]
  mov rdx, [rax + 0x18]
  mov rsi, [rax + 0x20]
  mov rdi, [rax + 0x28]

  mov r8, [rax + 0x30]
  mov r9, [rax + 0x38]
  mov r10, [rax + 0x40]
  mov r11, [rax + 0x48]
  mov r12, [rax + 0x50]
  mov r13, [rax + 0x58]
  mov r14, [rax + 0x60]
  mov r15, [rax + 0x68]

  push qword [rax + 0x90] ; SS
  push qword [rax + 0x88] ; RSP
  push qword [rax + 0x80] ; RFLAGS
  push qword [rax + 0x78] ; CS
  push qword [rax + 0x70] ; RIP

  mov rbp, [rax + 0x98]
  mov ds, [rax + 0xA0]
  mov es, [rax + 0xA8]
  mov fs, [rax + 0xB0]
  mov gs, [rax + 0xB8]

  mov rax, [rax + 0x00]

  iretq

global _proc_init
_proc_init:
  mov rbp, rsi ; Reset rbp
  mov rsp, rsi ; Move rsp to process stack
  jmp rdi ; Jump to process entry point

global _proc_use_kernel_stack
extern proc_kernel_stack
_proc_use_kernel_stack:
  pop rax
  mov rsp, proc_kernel_stack
  push rax
  ret
