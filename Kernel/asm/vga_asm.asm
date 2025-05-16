section .text

%define	VGA_AC_INDEX		0x3C0
%define	VGA_AC_WRITE		0x3C0
%define	VGA_AC_READ		0x3C1
%define	VGA_MISC_WRITE		0x3C2
%define VGA_SEQ_INDEX		0x3C4
%define VGA_SEQ_DATA		0x3C5
%define	VGA_MISC_READ		0x3CC
%define VGA_GC_INDEX 		0x3CE
%define VGA_GC_DATA 		0x3CF
%define VGA_CRTC_INDEX		0x3D4
%define VGA_CRTC_DATA		0x3D5
%define	VGA_INSTAT_READ		0x3DA

%define	VGA_NUM_SEQ_REGS	5
%define	VGA_NUM_CRTC_REGS	25
%define	VGA_NUM_GC_REGS		9
%define	VGA_NUM_AC_REGS		21
%define	VGA_NUM_REGS		(1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

global _vga_setmode
_vga_setmode:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx

    ; Write MISC register
    mov al, [rdi]
    mov dx, VGA_MISC_WRITE
    out dx, al
    inc rdi

    ; Write sequencer registers
    mov rcx, 0
.seq:
    mov rax, rcx
    mov dx, VGA_SEQ_INDEX
    out dx, al
    mov al, [rdi]
    mov dx, VGA_SEQ_DATA
    out dx, al
    inc rdi
    inc rcx
    cmp rcx, VGA_NUM_SEQ_REGS
    jl  .seq

    ; Unlock CRTC registers
    mov al, 0x03
    mov dx, VGA_CRTC_INDEX
    out dx, al
    mov dx, VGA_CRTC_DATA
    in  al, dx
    or  al, 0x80
    mov dx, VGA_CRTC_DATA
    out dx, al
    mov al, 0x11
    mov dx, VGA_CRTC_INDEX
    out dx, al
    mov dx, VGA_CRTC_DATA
    in  al, dx
    and al, 0x7f
    mov dx, VGA_CRTC_DATA
    out dx, al

    ; Make sure they remain unlocked (?)
    mov bl, [rdi + 0x03]
    or  bl, 0x80
    mov [rdi + 0x03], bl
    mov bl, [rdi + 0x11]
    and bl, 0x7f
    mov [rdi + 0x11], bl

    ; Write CRTC registers
    mov rcx, 0
.crtc:
    mov rax, rcx
    mov dx, VGA_CRTC_INDEX
    out dx, al
    mov al, [rdi]
    mov dx, VGA_CRTC_DATA
    out dx, al
    inc rdi
    inc rcx
    cmp rcx, VGA_NUM_CRTC_REGS
    jl  .crtc

    ; Write graphics controller registers
    mov rcx, 0
.gc:
    mov rax, rcx
    mov dx, VGA_GC_INDEX
    out dx, al
    mov al, [rdi]
    mov dx, VGA_GC_DATA
    out dx, al
    inc rdi
    inc rcx
    cmp rcx, VGA_NUM_GC_REGS
    jl  .gc

    ; Write attribute controller registers
    mov rcx, 0
.ac:
    mov dx, VGA_INSTAT_READ
    in  al, dx ; idk what this does
    mov rax, rcx
    mov dx, VGA_AC_INDEX
    out dx, al
    mov al, [rdi]
    mov dx, VGA_AC_WRITE
    out dx, al
    inc rdi
    inc rcx
    cmp rcx, VGA_NUM_AC_REGS
    jl  .ac
    
    ; Lock 16-color palette and unblank
    mov dx, VGA_INSTAT_READ
    in  al, dx ; idk what this does
    mov al, 0x20
    mov dx, VGA_AC_INDEX
    out dx, al

    pop rcx
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

global _vga_setplane
_vga_setplane:
    and rdi, 0x3

    mov al, 0x04
    mov dx, VGA_GC_INDEX
    out dx, al
    mov rax, rdi
    mov dx, VGA_GC_DATA
    out dx, al

    mov al, 0x02
    mov dx, VGA_SEQ_INDEX
    out dx, al
    ; al = 1 << rdi
    mov rax, 0x1
    mov rcx, rdi
    shl al, cl
    mov dx, VGA_SEQ_DATA
    out dx, al

    ret
