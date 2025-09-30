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

%macro push_state 0
  push rax
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15
%endmacro

%macro pop_state 0
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop rdi
  pop rsi
  pop rdx
  pop rcx
  pop rbx
  pop rax
%endmacro

; Generic exception handler
extern regdump_context
extern get_stack_base
%macro exception_handler 1
  ; Set register dump exception flag
  mov qword [regdump_context], 0x01

  ; Set register dump exception ID
  mov qword [regdump_context + 8], %1

  ; Memory dump
  push rax
  push rbx
  push rcx
  mov rax, [rsp + 8 * 3] ; RIP
  and rax, -16 ; Align to 16-byte boundary
  sub rax, 96 ; Go back a bit
  mov [regdump_context + 16], rax ; Store mem dump start address
  mov rbx, 0
.loop:
  mov cl, [rax]
  mov [regdump_context + 24 + rbx], cl
  inc rax
  inc rbx
  cmp rbx, 128
  jne .loop
  pop rcx
  pop rbx
  pop rax

  call _regdump

  ; Restart to userland
  call get_stack_base
  mov [rsp + 8 * 3], rax        ; Reset stack pointer
  mov qword [rsp], userland     ; Userland entry point

  iretq
%endmacro

; -----------------------------------------------------------------------------
; Rutinas de PIC
; -----------------------------------------------------------------------------

; Establece la máscara del PIC maestro
global _pic_master_mask
_pic_master_mask:
  mov rax, rdi
  out 0x21, al
  ret

; Establece la máscara del PIC esclavo
global _pic_slave_mask
_pic_slave_mask:
  mov rax, rdi
  out 0xA1, al
  ret

; -----------------------------------------------------------------------------
; Interrupt handlers
; -----------------------------------------------------------------------------

;------------------------------------------------------------------------------
; Timer tick (IRQ 0)
;------------------------------------------------------------------------------

global _irq_00_handler
extern irq_timer_handler
_irq_00_handler:
  push_state

  mov rdi, rsp
  call irq_timer_handler
  mov rsp, rax

  mov al, 0x20
  out 0x20, al

  pop_state

  iretq

;------------------------------------------------------------------------------
; Keyboard (IRQ 1)
; Special handling to alter CPU state as little as possible
; for state dump function
;------------------------------------------------------------------------------

global _irq_01_handler
extern kbd_add_key_event
extern _regdump
_irq_01_handler:
  cli
  push rax     ; Preserve RAX value for statedump

  mov rax, 0
  in  al, 0x60

  ; Signal PIC EOI (End of Interrupt)
  push rax
  mov al, 0x20
  out 0x20, al
  pop rax

  cmp al, 0x3B ; if F1 is pressed, dump registers
  jne .keyEvent

  pop rax
  call _regdump
  jmp .exit

.keyEvent:
  pushall

  mov rdi, rax
  call kbd_add_key_event

  popall
  pop rax

.exit:
  sti
  iretq

;------------------------------------------------------------------------------
; Syscall handler (IRQ 80h)
;------------------------------------------------------------------------------

global _irq_80_handler
extern syscall_dispatch_table
_irq_80_handler:
  push rbp
  mov rbp, rsp

  sti ; Allow syscalls to be interrupted

  mov rax, [syscall_dispatch_table + rax * 8]
  call rax

  mov rsp, rbp
  pop rbp

  iretq

;------------------------------------------------------------------------------
; Exceptions
;------------------------------------------------------------------------------

; Division by Zero (0x00)
global _exception_00_handler
_exception_00_handler:
  exception_handler 0x00

; Invalid Opcode (0x06)
global _exception_06_handler
_exception_06_handler:
  exception_handler 0x06

;------------------------------------------------------------------------------
; Register dump function
;------------------------------------------------------------------------------

extern register_state
extern show_cpu_state
_regdump:
    cli

    push rax
    mov [register_state + 0x00], rax
    mov [register_state + 0x08], rbx
    mov [register_state + 0x10], rcx
    mov [register_state + 0x18], rdx

    mov [register_state + 0x20], rsi
    mov [register_state + 0x28], rdi
    mov rax, [rsp + 8 * 5]  ; RSP (before jumping into IRQ handler)
    mov [register_state + 0x30], rax
    mov [register_state + 0x38], rbp

    mov [register_state + 0x40], r8
    mov [register_state + 0x48], r9
    mov [register_state + 0x50], r10
    mov [register_state + 0x58], r11
    mov [register_state + 0x60], r12
    mov [register_state + 0x68], r13
    mov [register_state + 0x70], r14
    mov [register_state + 0x78], r15

    mov rax, [rsp + 8 * 2] ; RIP (before jumping into IRQ handler)
    mov [register_state + 0x80], rax

    mov rax, [rsp + 8 * 4] ; RFLAGS
    mov [register_state + 0x88], rax

    mov rax, [rsp + 8 * 3] ; CS
    mov [register_state + 0xB8], ax
    mov rax, [rsp + 8 * 6] ; SS
    mov [register_state + 0xBA], ax
    mov [register_state + 0xBC], ds
    mov [register_state + 0xBE], es
    mov [register_state + 0xC0], fs
    mov [register_state + 0xC2], gs

    sti

    pushall
    call show_cpu_state
    popall
    pop rax

    ret

section .rodata
kernel      equ 0x100000
userland    equ 0x400000
