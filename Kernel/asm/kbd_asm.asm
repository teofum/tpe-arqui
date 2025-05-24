section .text

global _kbd_read
_kbd_read:
    mov rax,0
    in al, 60h
    ret
