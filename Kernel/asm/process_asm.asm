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

global _proc_init
_proc_init:
  mov rbp, rsi ; Reset rbp
  mov rsp, rsi ; Move rsp to process stack
  jmp rdi ; Jump to process entry point
