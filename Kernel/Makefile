include Makefile.inc

KERNEL=out/kernel.bin
SOURCES=$(wildcard *.c)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(foreach I,$(SOURCES:.c=.o),build/$I)
OBJECTS_ASM=$(foreach I,$(notdir $(SOURCES_ASM:.asm=.o)),build/$I)
LOADERSRC=loader.asm

LOADEROBJECT=$(foreach I,$(LOADERSRC:.asm=.o),build/$I)
STATICLIBS=

all: $(KERNEL)

$(KERNEL): out $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

build/%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

build/%.o: asm/%.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT): build
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

build:
	mkdir build

out:
	mkdir out

clean:
	rm -rf build out asm/*.o *.o *.bin

.PHONY: all clean
