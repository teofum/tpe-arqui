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


GLOBAL _hlt
_hlt:
  sti
	hlt
	ret

GLOBAL _cli
_cli:
	cli
	ret


GLOBAL _sti
_sti:
	sti
	ret


GLOBAL haltcpu
haltcpu:
	cli
	hlt
	ret


GLOBAL picMasterMask
picMasterMask:
  mov rax, rdi
  out	21h, al
  ret

GLOBAL picSlaveMask
picSlaveMask:
  mov rax, rdi  ; ax = mascara de 16 bits
  out	0A1h, al
  ret

;8254 Timer (Timer Tick)
GLOBAL _irq00Handler
_irq00Handler:
	irqHandlerMaster 0

;Keyboard
;GLOBAL _irq01Handler
;_irq01Handler:
;	irqHandlerMaster 1

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
;	exceptionHandler 0
