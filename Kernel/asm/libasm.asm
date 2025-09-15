section .text

; -----------------------------------------------------------------------------
; Rutinas de CPU
; -----------------------------------------------------------------------------

; Identifica el vendor de la CPU
global cpu_vendor
cpu_vendor:
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

;------------------------------------------------------------------------------
; uint8_t inb(uint16_t port)
;------------------------------------------------------------------------------
global inb
inb:
  mov dx, di        ; El primer parámetro (port) está en DI (16 bits)
  xor rax, rax      ; Limpiar rax
  in al, dx         ; Leer byte del puerto en AL
  ret

;------------------------------------------------------------------------------
; void outb(uint16_t port, uint8_t value)
;------------------------------------------------------------------------------
global outb
outb:
  mov dx, di        ; El primer parámetro (port) está en DI (16 bits)
  mov al, sil       ; El segundo parámetro (value) está en SI (8 bits)
  out dx, al        ; Escribir AL al puerto DX
  ret
;------------------------------------------------------------------------------
