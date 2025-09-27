include Makefile.inc

KERNEL=out/kernel.bin
DEBUG_ELF=$(KERNEL:.bin=.elf)
SOURCES=$(wildcard *.c)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(foreach I,$(SOURCES:.c=.o),build/$I)
OBJECTS_ASM=$(foreach I,$(notdir $(SOURCES_ASM:.asm=.o)),build/$I)
LOADERSRC=loader.asm

LOADEROBJECT=$(foreach I,$(LOADERSRC:.asm=.o),build/$I)
STATICLIBS=

all: $(KERNEL) $(DEBUG_ELF)

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM) | out
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

$(DEBUG_ELF): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM) | out
	$(LD) $(LDFLAGS) -T kernel.ld --oformat=elf64-x86-64 -o $(DEBUG_ELF) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

build/%.o: %.c | build
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

build/%.o: asm/%.asm | build
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT): | build
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

build:
	mkdir -p build

out:
	mkdir -p out

clean:
	rm -rf build out

.PHONY: clean
