section .text

global _proc_init
_proc_init:
    mov rbp, rsi ; Reset rbp
    mov rsp, rsi ; Move rsp to process stack
    call rdi ; Jump to process entry point

global _proc_timer_interrupt
_proc_timer_interrupt:
    int 0x20
    ret
