section .text

global _syscall
_syscall:
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov rcx, r8
    mov r8, r9

    int 0x80
    
    ret

global sys_vga_clear
sys_vga_clear:
    mov rax, 0x20
    int 0x80
    ret

global sys_vga_pixel
sys_vga_pixel:
    mov rax, 0x21
    int 0x80
    ret

global sys_vga_line
sys_vga_line:
    mov rax, 0x22
    int 0x80
    ret

global sys_vga_rect
sys_vga_rect:
    mov rax, 0x23
    int 0x80
    ret

global sys_vga_frame
sys_vga_frame:
    mov rax, 0x24
    int 0x80
    ret

global sys_vga_shade
sys_vga_shade:
    mov rax, 0x25
    int 0x80
    ret

global sys_vga_gradient
sys_vga_gradient:
    mov rax, 0x26
    int 0x80
    ret

global sys_vga_font
sys_vga_font:
    mov rax, 0x27
    int 0x80
    ret

global sys_vga_text
sys_vga_text:
    mov rax, 0x28
    int 0x80
    ret

global sys_vga_textWrap
sys_vga_textWrap:
    mov rax, 0x29
    int 0x80
    ret

global sys_vga_present
sys_vga_present:
    mov rax, 0x2A
    int 0x80
    ret

global sys_vga_setFramebuffer
sys_vga_setFramebuffer:
    mov rax, 0x2B
    int 0x80
    ret

