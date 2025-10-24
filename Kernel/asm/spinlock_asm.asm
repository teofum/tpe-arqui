section .text
global lock_acquire, lock_release

lock_acquire:
    mov eax, 1
.spin:
    xchg eax, [rdi]    ; atomic swap
    test eax, eax
    jnz .spin
    ret

lock_release:
    mov dword [rdi], 0
    ret
