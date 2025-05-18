GLOBAL _kbd_read
GLOBAL _kbd_readState

section .text



_kbd_read:
    mov rax,0

.getState:
    in al, 64h
    test al, 1
    jz .getState  ;no esta listo .-.
.getKey:
    in ax, 60h
ret

_kbd_readState:
    mov rax,0
    in al, 64h
    ret