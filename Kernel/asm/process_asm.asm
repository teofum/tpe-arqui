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
extern proc_get_registers_addr_for_current_process
_proc_jump_to_spawned:
  push rbx                                              ; backup rbx
  pushall                                               ; backup regs that a c function can clobber
  call proc_get_registers_addr_for_current_process
  mov rbx, rax                                          ; mov regs addr to rbx so we can store rax
  popall                                                ; restore all regs (not rbx)

  mov [rbx], rax                                        ; store rax in regs struct pcb
  mov rax, rbx                                          ; mov regs addr to rax
  pop rbx                                               ; restore rbx

  ; Fill out the rest of pcb registers
  mov [rax + 0x08], rbx
  mov [rax + 0x10], rcx
  mov [rax + 0x18], rdx
  mov [rax + 0x20], rsi
  mov [rax + 0x28], rdi

  mov [rax + 0x30], r8
  mov [rax + 0x38], r9
  mov [rax + 0x40], r10
  mov [rax + 0x48], r11
  mov [rax + 0x50], r12
  mov [rax + 0x58], r13
  mov [rax + 0x60], r14
  mov [rax + 0x68], r15

  ; Get RIP from the last iretq frame
  mov rbx, [last_iretq_frame]
  mov rbx, [rbx]
  mov [rax + 0x70], rbx
  ; pop qword [rax + 0x78] ; CS

  pushf
  pop qword [rax + 0x80] ; RFLAGS

  ; pop qword [rax + 0x88] ; RSP
  ; pop qword [rax + 0x90] ; SS

  mov [rax + 0x98], rbp
  mov [rax + 0xA0], ds
  mov [rax + 0xA8], es
  mov [rax + 0xB0], fs
  mov [rax + 0xB8], gs

  mov rbp, rsi ; Reset rbp
  mov rsp, rsi ; Move rsp to process stack
  jmp rdi ; Jump to process entry point

global _proc_init
_proc_init:
  mov rbp, rsi ; Reset rbp
  mov rsp, rsi ; Move rsp to process stack
  jmp rdi ; Jump to process entry point
