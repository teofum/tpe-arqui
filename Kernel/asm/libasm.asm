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

global _regdump
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

    mov rax, cr0
    mov [registerState + 0x90], rax
    mov rax, cr2
    mov [registerState + 0x98], rax
    mov rax, cr3
    mov [registerState + 0xA0], rax
    mov rax, cr4
    mov [registerState + 0xA8], rax
    mov rax, cr8
    mov [registerState + 0xB0], rax

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
