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

%macro define_syscall 2
global %1
%1:
    mov rax, %2
    int 0x80
    ret
%endmacro

define_syscall kbd_poll_events, 0x10
define_syscall kbd_keydown, 0x11
define_syscall kbd_keypressed, 0x12
define_syscall kbd_keyreleased, 0x13
define_syscall kbd_get_key_event, 0x14
define_syscall kbd_getchar, 0x15

define_syscall vga_clear, 0x20
define_syscall vga_pixel, 0x21
define_syscall vga_line, 0x22
define_syscall vga_rect, 0x23
define_syscall vga_frame, 0x24
define_syscall vga_shade, 0x25
define_syscall vga_gradient, 0x26
define_syscall vga_font, 0x27
define_syscall vga_text, 0x28
define_syscall vga_text_wrap, 0x29
define_syscall vga_present, 0x2A
define_syscall vga_set_framebuffer, 0x2B
define_syscall vga_copy, 0x2C
define_syscall vga_copy2x, 0x2D
define_syscall vga_bitmap, 0x2E
define_syscall vga_get_vbe_info, 0x2F

define_syscall gfx_clear, 0xA0
define_syscall gfx_draw_primitives, 0xA1
define_syscall gfx_draw_primitives_indexed, 0xA2
define_syscall gfx_draw_wireframe, 0xA3
define_syscall gfx_draw_wireframe_indexed, 0xA4
define_syscall gfx_set_buffers, 0xA5
define_syscall gfx_copy, 0xA6
define_syscall gfx_depthcopy, 0xA7
define_syscall gfx_load_model, 0xA8
define_syscall gfx_set_light, 0xAA
define_syscall gfx_set_light_type, 0xAB
define_syscall gfx_set_matrix, 0xAC
define_syscall gfx_set_flag, 0xAD
define_syscall gfx_get_framebuffer, 0xAE
define_syscall gfx_present, 0xAF

define_syscall audio_beep, 0x30
define_syscall audio_play_melody, 0x31
