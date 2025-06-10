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
extern irqDispatcher
%macro irqHandlerMaster 1
	pushall
  push rbp
  mov rbp, rsp

	mov rdi, %1
	call irqDispatcher

	; Signal PIC EOI (End of Interrupt)
	mov al, 0x20
	out 0x20, al

  mov rsp, rbp
  pop rbp
	popall
	iretq
%endmacro

; Generic exception handler
extern regdumpContext
extern getStackBase
%macro exceptionHandler 1
  push rax

  ; Set register dump exception flag
  mov qword [regdumpContext], 0x01

  ; Set register dump exception ID
  mov qword [regdumpContext + 8], %1

  ; Push interrupt-stored registers to stack
  mov rax, [rsp + 8 * 1]  ; RIP
  mov [regdumpContext + 16], rax ; Store RIP temporarily
  push rax
  mov rax, [rsp + 8 * 3]  ; CS
  push rax
  mov rax, [rsp + 8 * 5]  ; RFLAGS
  push rax
  mov rax, [rsp + 8 * 7]  ; RSP original
  push rax
  mov rax, [rsp + 8 * 9]  ; SS
  push rax

  ; Memory dump
  push rbx
  push rcx
  mov rax, [regdumpContext + 16] ; RIP
  and rax, -16 ; Align to 16-byte boundary
  sub rax, 96 ; Go back a bit
  mov [regdumpContext + 16], rax ; Store mem dump start address
  mov rbx, 0
.loop:
  mov cl, [rax]
  mov [regdumpContext + 24 + rbx], cl
  inc rax
  inc rbx
  cmp rbx, 128
  jne .loop
  pop rcx
  pop rbx

  call _regdump

  ; Restart to userland
  add rsp, 48                   ; yeet the stack
  call getStackBase
  mov [rsp + 8 * 3], rax        ; Reset stack pointer
  mov qword [rsp], userland     ; Userland entry point

  iretq
%endmacro

; -----------------------------------------------------------------------------
; Rutinas de PIC
; -----------------------------------------------------------------------------

; Establece la máscara del PIC maestro
global _picMasterMask
_picMasterMask:
  mov rax, rdi
  out 0x21, al
  ret

; Establece la máscara del PIC esclavo
global _picSlaveMask
_picSlaveMask:
  mov rax, rdi
  out 0xA1, al
  ret

; -----------------------------------------------------------------------------
; Handlers de interrupciones
; -----------------------------------------------------------------------------

;------------------------------------------------------------------------------
; Timer tick (IRQ 0)
;------------------------------------------------------------------------------

global _irq00Handler
_irq00Handler:
	irqHandlerMaster 0

;------------------------------------------------------------------------------
; Keyboard handler
; Special handling to alter CPU state as little as possible
; for state dump function
;------------------------------------------------------------------------------

global _irq01Handler
extern kbd_addKeyEvent
extern _regdump
_irq01Handler:
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

  mov rax, [rsp + 8 * 1] ; RIP
  push rax
  mov rax, [rsp + 8 * 3] ; CS
  push rax
  mov rax, [rsp + 8 * 5] ; RFLAGS
  push rax
  mov rax, [rsp + 8 * 7] ; RSP (old)
  push rax
  mov rax, [rsp + 8 * 9] ; SS
  push rax

  call _regdump
  add rsp, 40 ; yeet the stack
  jmp .exit

.keyEvent:
	pushall

  mov rdi, rax
  call kbd_addKeyEvent

  popall

.exit:
  pop rax
	iretq

;------------------------------------------------------------------------------
; Syscall handler (IRQ 80h)
;------------------------------------------------------------------------------

global _irq80Handler
extern syscallDispatchTable
_irq80Handler:
  push rbp
  mov rbp, rsp

  sti ; Allow syscalls to be interrupted

  mov rax, [syscallDispatchTable + rax * 8]
  call rax

  mov rsp, rbp
  pop rbp
  iretq

;------------------------------------------------------------------------------
; Exceptions
;------------------------------------------------------------------------------

; Division by Zero (0x00)
global _exception00Handler
_exception00Handler:
  exceptionHandler 0x00

; Invalid Opcode (0x06)
global _exception06Handler
_exception06Handler:
  exceptionHandler 0x06

;------------------------------------------------------------------------------
; Register dump function
;------------------------------------------------------------------------------

extern registerState
extern showCPUState
_regdump:
    cli

    mov rax, [rsp + 8 * 6]
    mov [registerState + 0x00], rax
    mov [registerState + 0x08], rbx
    mov [registerState + 0x10], rcx
    mov [registerState + 0x18], rdx

    mov [registerState + 0x20], rsi
    mov [registerState + 0x28], rdi
    mov rax, [rsp + 8 * 2]  ; RSP (before jumping into IRQ handler)
    mov [registerState + 0x30], rax
    mov [registerState + 0x38], rbp

    mov [registerState + 0x40], r8
    mov [registerState + 0x48], r9
    mov [registerState + 0x50], r10
    mov [registerState + 0x58], r11
    mov [registerState + 0x60], r12
    mov [registerState + 0x68], r13
    mov [registerState + 0x70], r14
    mov [registerState + 0x78], r15

    mov rax, [rsp + 8 * 5] ; RIP (before jumping into IRQ handler)
    mov [registerState + 0x80], rax

    mov rax, [rsp + 8 * 3] ; RFLAGS
    mov [registerState + 0x88], rax

    mov rax, [rsp + 8 * 4] ; CS
    mov [registerState + 0xB8], ax
    mov rax, [rsp + 8 * 1] ; SS
    mov [registerState + 0xBA], ax
    mov [registerState + 0xBC], ds
    mov [registerState + 0xBE], es
    mov [registerState + 0xC0], fs
    mov [registerState + 0xC2], gs

    sti

    pushall
    call showCPUState
    popall

    ret

section .rodata
kernel      equ 0x100000
userland    equ 0x2000000
