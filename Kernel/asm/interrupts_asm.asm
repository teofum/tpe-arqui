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

; Generic IRQ handler
%macro irq_handler_begin 0
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

  pop qword [rax + 0x70] ; RIP
  pop qword [rax + 0x78] ; CS
  pop qword [rax + 0x80] ; RFLAGS
  pop qword [rax + 0x88] ; RSP
  pop qword [rax + 0x90] ; SS

  mov [rax + 0x98], rbp
  mov [rax + 0xA0], ds
  mov [rax + 0xA8], es
  mov [rax + 0xB0], fs
  mov [rax + 0xB8], gs

  ; Move to kernel stack for interrupt handler
  mov rsp, proc_kernel_stack
%endmacro

%macro irq_handler_end 0
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

  ; Signal PIC EOI (End of Interrupt)
  push rax
  mov al, 0x20
  out 0x20, al
  pop rax

  iretq
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
; Handlers de interrupciones
; -----------------------------------------------------------------------------

extern irq_dispatcher

;------------------------------------------------------------------------------
; Timer tick (IRQ 0)
;------------------------------------------------------------------------------

global _irq_00_handler
extern proc_kernel_stack
extern proc_get_registers_addr_for_current_process
_irq_00_handler:
  irq_handler_begin
  mov rdi, 0
  call irq_dispatcher
  irq_handler_end

;------------------------------------------------------------------------------
; Keyboard handler
; Special handling to alter CPU state as little as possible
; for state dump function
;------------------------------------------------------------------------------

global _irq_01_handler
extern kbd_add_key_event
extern _regdump
_irq_01_handler:
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
