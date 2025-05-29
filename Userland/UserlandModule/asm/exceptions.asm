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
