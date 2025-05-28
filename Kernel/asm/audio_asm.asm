section .text

;------------------------------------------------------------------------------
; uint8_t inb(uint16_t port)
;------------------------------------------------------------------------------
global inb
inb:
  mov dx, di        ; El primer parámetro (port) está en DI (16 bits)
  xor eax, eax      ; Limpiar eax
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
