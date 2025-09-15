GLOBAL _throw_00
GLOBAL _throw_06

section .text

_throw_00:
  mov rax, 0
  div rax
  ret

_throw_06:
  ud2
  ret

; Note: this function will clobber *all* the registers
; we assume the kernel will restart immediately after, so we don't care
global _regdump_test
_regdump_test:
  ; Fill registers with known values we can look for in the dump
  mov rax, 0xaaaaaaaaaaaaaaaa
  mov rbx, 0xbbbbbbbbbbbbbbbb
  mov rcx, 0xcccccccccccccccc
  mov rdx, 0xdddddddddddddddd
  mov rdi, 0xdeadbeefdeadbeef
  mov rsi, 0x4242424242424242
  mov rbp, 0x1020304050607080

  mov r8, 0x8888888888888888
  mov r9, 0x9999999999999999
  mov r10, 0x1010101010101010
  mov r11, 0x1111111111111111
  mov r12, 0x1212121212121212
  mov r13, 0x1313131313131313
  mov r14, 0x1414141414141414
  mov r15, 0x1515151515151515
  
  ; Call an illegal instruction to immediately trigger a register dump
  ud2
  ret; probably not needed
