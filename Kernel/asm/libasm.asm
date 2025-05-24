section .text

; -----------------------------------------------------------------------------
; Rutinas de CPU
; -----------------------------------------------------------------------------

; Identifica el vendor de la CPU
global cpuVendor
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

global _hlt
_hlt:
    sti
	hlt
	ret

; Deshabilita interrupciones
global _cli
_cli:
	cli
	ret

; Habilita interrupciones
global _sti
_sti:
	sti
	ret

; Termina la ejecución de la CPU
global haltcpu
haltcpu:
	cli
	hlt
	ret

