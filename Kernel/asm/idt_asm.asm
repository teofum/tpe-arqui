section .text

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

; Macro para handlers de IRQ
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
    mov rax, rdi  ; ax = mascara de 16 bits
    out 0xa1, al
    ret

; -----------------------------------------------------------------------------
; Handlers de interrupciones
; -----------------------------------------------------------------------------

; Timer tick (IRQ 0)
global _irq00Handler
_irq00Handler:
	irqHandlerMaster 0

; Keyboard handler
; Special handling to alter CPU state as little as possible
; for state dump function
global _irq01Handler
extern kbd_addKeyEvent
extern _regdump
_irq01Handler:
	pushall

    mov rax, 0
    in  al, 0x60

	; Signal PIC EOI (End of Interrupt)
    push rax
	mov al, 0x20
	out 0x20, al
    pop rax

    cmp al, 0x3B ; if F1 is pressed, dump registers
    jne .keyEvent

    call _regdump
    jmp .exit

.keyEvent:
    mov rdi, rax
    call kbd_addKeyEvent

.exit:
	popall
	iretq

; Syscall handler (IRQ 80h)
global _irq80Handler
extern syscallDispatchTable
_irq80Handler:
  push rbp
  mov rbp, rsp

  mov rax, [syscallDispatchTable + rax * 8]
  call rax

  mov rsp, rbp
  pop rbp
  iretq

; Exception handlers (comentados por ahora)
;EXTERN exceptionDispatcher
;%macro exceptionHandler 1
;	pushState
;
;	mov rdi, %1 ; pasaje de parametro
;	call exceptionDispatcher
;
;	popState
;	iretq
;%endmacro

;Zero Division Exception
;GLOBAL _exception0Handler
;_exception0Handler:
;	exceptionHandler 0ret
