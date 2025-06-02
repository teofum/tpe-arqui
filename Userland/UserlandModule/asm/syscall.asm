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

global kbd_pollEvents
kbd_pollEvents:
    mov rax, 0x10
    int 0x80
    ret

global kbd_keydown
kbd_keydown:
    mov rax, 0x11
    int 0x80
    ret

global kbd_keypressed
kbd_keypressed:
    mov rax, 0x12
    int 0x80
    ret

global kbd_keyreleased
kbd_keyreleased:
    mov rax, 0x13
    int 0x80
    ret

global kbd_getKeyEvent
kbd_getKeyEvent:
    mov rax, 0x14
    int 0x80
    ret

global kbd_getchar
kbd_getchar:
    mov rax, 0x15
    int 0x80
    ret

global vga_clear
vga_clear:
    mov rax, 0x20
    int 0x80
    ret

global vga_pixel
vga_pixel:
    mov rax, 0x21
    int 0x80
    ret

global vga_line
vga_line:
    mov rax, 0x22
    int 0x80
    ret

global vga_rect
vga_rect:
    mov rax, 0x23
    int 0x80
    ret

global vga_frame
vga_frame:
    mov rax, 0x24
    int 0x80
    ret

global vga_shade
vga_shade:
    mov rax, 0x25
    int 0x80
    ret

global vga_gradient
vga_gradient:
    mov rax, 0x26
    int 0x80
    ret

global vga_font
vga_font:
    mov rax, 0x27
    int 0x80
    ret

global vga_text
vga_text:
    mov rax, 0x28
    int 0x80
    ret

global vga_textWrap
vga_textWrap:
    mov rax, 0x29
    int 0x80
    ret

global vga_present
vga_present:
    mov rax, 0x2A
    int 0x80
    ret

global vga_setFramebuffer
vga_setFramebuffer:
    mov rax, 0x2B
    int 0x80
    ret

global vga_copy
vga_copy:
    mov rax, 0x2C
    int 0x80
    ret

global vga_copy2x
vga_copy2x:
    mov rax, 0x2D
    int 0x80
    ret

global gfx_clear
gfx_clear:
    mov rax, 0xA0
    int 0x80
    ret

global gfx_drawPrimitives
gfx_drawPrimitives:
    mov rax, 0xA1
    int 0x80
    ret

global gfx_drawPrimitivesIndexed
gfx_drawPrimitivesIndexed:
    mov rax, 0xA2
    int 0x80
    ret

global gfx_drawWireframe
gfx_drawWireframe:
    mov rax, 0xA3
    int 0x80
    ret

global gfx_drawWireframeIndexed
gfx_drawWireframeIndexed:
    mov rax, 0xA4
    int 0x80
    ret

global gfx_parseObj
gfx_parseObj:
    mov rax, 0xA9
    int 0x80
    ret

global gfx_setLight
gfx_setLight:
    mov rax, 0xAA
    int 0x80
    ret

global gfx_setLightType
gfx_setLightType:
    mov rax, 0xAB
    int 0x80
    ret

global gfx_setMatrix
gfx_setMatrix:
    mov rax, 0xAC
    int 0x80
    ret

global gfx_setRenderResolution
gfx_setRenderResolution:
    mov rax, 0xAD
    int 0x80
    ret

global gfx_present
gfx_present:
    mov rax, 0xAF
    int 0x80
    ret
