GCC=x86_64-linux-gnu-gcc
GPP=x86_64-linux-gnu-g++
LD=x86_64-linux-gnu-ld
AR=x86_64-linux-gnu-ar
ASM=nasm

KERNEL_GCCFLAGS=-m64 -g -F dwarf -fno-exceptions -fno-asynchronous-unwind-tables -mno-sse2 -fno-builtin-malloc -fno-builtin-free -fno-builtin-realloc -mno-red-zone -Wall -ffreestanding -nostdlib -fno-common -std=c99 -fno-pie -O1
KERNEL_LDFLAGS=--warn-common -z max-page-size=0x1000
GCCFLAGS=-m64 -g -F dwarf -fno-exceptions -std=c99 -Wall -ffreestanding -nostdlib -fno-common -mno-red-zone -mno-sse2 -fno-builtin-malloc -fno-builtin-free -fno-builtin-realloc -fno-pie
ARFLAGS=rvs
ASMFLAGS=-felf64
