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

    mov [registerState + 0x00], rax
    mov [registerState + 0x08], rbx
    mov [registerState + 0x10], rcx
    mov [registerState + 0x18], rdx

    mov [registerState + 0x20], rsi
    mov [registerState + 0x28], rdi
    mov [registerState + 0x30], rsp
    mov [registerState + 0x38], rbp

    mov [registerState + 0x40], r8
    mov [registerState + 0x48], r9
    mov [registerState + 0x50], r10
    mov [registerState + 0x58], r11
    mov [registerState + 0x60], r12
    mov [registerState + 0x68], r13
    mov [registerState + 0x70], r14
    mov [registerState + 0x78], r15

    ; abuse a call instruction to read RIP
    call .cursed ; This pushes RIP onto the stack
.cursed:
    pop rax ; We then pop into RAX so we can MOV it to memory
    mov [registerState + 0x80], rax

    ; get the flags register
    pushfq
    pop rax
    mov [registerState + 0x88], rax

    ; TODO figure out how to read control registers
    ;mov [registerState + 0x90], cr0
    ;mov [registerState + 0x98], cr2
    ;mov [registerState + 0xA0], cr3
    ;mov [registerState + 0xA8], cr4
    ;mov [registerState + 0xB0], cr8

    mov [registerState + 0xB8], cs
    mov [registerState + 0xBA], ss
    mov [registerState + 0xBC], ds
    mov [registerState + 0xBE], es
    mov [registerState + 0xC0], fs
    mov [registerState + 0xC2], gs

    sti

    call showCPUState

    ret
