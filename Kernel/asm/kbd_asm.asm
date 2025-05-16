GLOBAL getKey
GLOBAL getKBDState

section .text



_kbd_read
    mov rax,0

.getState:
    in al, 64h
    test al, 1
    jz .getState  ;no esta listo .-.
.getKey:
    in ax, 60h
ret
