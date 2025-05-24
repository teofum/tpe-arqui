section .text

global _kbd_irqHandler
extern kbd_addKeyEvent
extern _regdump
_kbd_irqHandler:
    mov rax, 0
    in  al, 0x60
    cmp al, 0x3B ; if F1 is pressed, dump registers
    jne .keyEvent

    call _regdump
    ret

.keyEvent:
    mov rdi, rax
    call kbd_addKeyEvent
    ret
