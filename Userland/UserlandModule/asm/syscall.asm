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

%macro defineSyscall 2
global %1
%1:
    mov rax, %2
    int 0x80
    ret
%endmacro

defineSyscall kbd_pollEvents, 0x10
defineSyscall kbd_keydown, 0x11
defineSyscall kbd_keypressed, 0x12
defineSyscall kbd_keyreleased, 0x13
defineSyscall kbd_getKeyEvent, 0x14
defineSyscall kbd_getchar, 0x15

defineSyscall vga_clear, 0x20
defineSyscall vga_pixel, 0x21
defineSyscall vga_line, 0x22
defineSyscall vga_rect, 0x23
defineSyscall vga_frame, 0x24
defineSyscall vga_shade, 0x25
defineSyscall vga_gradient, 0x26
defineSyscall vga_font, 0x27
defineSyscall vga_text, 0x28
defineSyscall vga_textWrap, 0x29
defineSyscall vga_present, 0x2A
defineSyscall vga_setFramebuffer, 0x2B
defineSyscall vga_copy, 0x2C
defineSyscall vga_copy2x, 0x2D
defineSyscall vga_bitmap, 0x2E

defineSyscall gfx_clear, 0xA0
defineSyscall gfx_drawPrimitives, 0xA1
defineSyscall gfx_drawPrimitivesIndexed, 0xA2
defineSyscall gfx_drawWireframe, 0xA3
defineSyscall gfx_drawWireframeIndexed, 0xA4
defineSyscall gfx_setBuffers, 0xA5
defineSyscall gfx_copy, 0xA6
defineSyscall gfx_depthcopy, 0xA7
defineSyscall gfx_parseObj, 0xA9
defineSyscall gfx_setLight, 0xAA
defineSyscall gfx_setLightType, 0xAB
defineSyscall gfx_setMatrix, 0xAC
defineSyscall gfx_setFlag, 0xAD
defineSyscall gfx_getFramebuffer, 0xAE
defineSyscall gfx_present, 0xAF

defineSyscall audio_beep, 0x30
defineSyscall audio_play_melody, 0x31
