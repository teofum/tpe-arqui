section .text
global lock_acquire, lock_release

lock_acquire:
    mov rax, 1
.spin:
    xchg rax, [rdi]    ; atomic swap
    test rax, rax
    jnz .spin
    ret

lock_release:
    mov qword [rdi], 0
    ret
