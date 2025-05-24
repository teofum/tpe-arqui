section .text

global _kbd_read
_kbd_read:
    mov rax, 0
    in  al, 0x60
    ret
