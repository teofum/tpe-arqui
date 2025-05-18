SECTION .text

%macro pushall 0
	push rbx
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popall 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx
%endmacro

; Macro para handlers de IRQ
EXTERN irqDispatcher
%macro irqHandlerMaster 1
	pushall
    push rbp
    mov rbp, rsp

	mov rdi, %1 ; pasaje de parametro
	call irqDispatcher

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

    mov rsp, rbp
    pop rbp
	popall
	iretq
%endmacro

; -----------------------------------------------------------------------------
; Rutinas de CPU
; -----------------------------------------------------------------------------

; Identifica el vendor de la CPU
GLOBAL cpuVendor
cpuVendor:
	push rbp
	mov rbp, rsp

	push rbx

	mov rax, 0
	cpuid

	mov [rdi], ebx
	mov [rdi + 4], edx
	mov [rdi + 8], ecx

	mov byte [rdi+13], 0

	mov rax, rdi

	pop rbx

	mov rsp, rbp
	pop rbp
	ret


GLOBAL _hlt
_hlt:
  sti
	hlt
	ret

; Deshabilita interrupciones
GLOBAL _cli
_cli:
	cli
	ret

; Habilita interrupciones
GLOBAL _sti
_sti:
	sti
	ret

; Termina la ejecución de la CPU
GLOBAL haltcpu
haltcpu:
	cli
	hlt
	ret

; -----------------------------------------------------------------------------
; Rutinas de PIC
; -----------------------------------------------------------------------------

; Establece la máscara del PIC maestro
GLOBAL picMasterMask
picMasterMask:
  mov rax, rdi
  out	21h, al
  ret

; Establece la máscara del PIC esclavo
GLOBAL picSlaveMask
picSlaveMask:
  mov rax, rdi  ; ax = mascara de 16 bits
  out	0A1h, al
  ret

; -----------------------------------------------------------------------------
; Handlers de interrupciones
; -----------------------------------------------------------------------------

; Timer tick (IRQ 0)
GLOBAL _irq00Handler
_irq00Handler:
	irqHandlerMaster 0

; Keyboard handler (comentado por ahora)
GLOBAL _irq01Handler
_irq01Handler:
	irqHandlerMaster 1

; Syscall handler (IRQ 80h)
GLOBAL _irq80Handler
EXTERN syscallDispatchTable
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
