include ../Makefile.inc

MODULE=out/userlandModule.bin
DEBUG_ELF=$(MODULE:.bin=.elf)
SOURCES=$(wildcard [^_]*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(foreach I,$(SOURCES:.c=.o),build/$I)
OBJECTS_ASM=$(foreach I,$(notdir $(SOURCES_ASM:.asm=.o)),build/$I)
LOADERSRC=_loader.c

LOADEROBJECT=$(foreach I,$(LOADERSRC:.c=.o),build/$I)

all: $(MODULE) $(DEBUG_ELF)

$(MODULE): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM) | out
	$(LD) $(LDFLAGS) -T userlandModule.ld -o $(MODULE) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM)

$(DEBUG_ELF): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM) | out
	$(LD) $(LDFLAGS) -T userlandModule.ld --oformat=elf64-x86-64 -o $(DEBUG_ELF) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM)

build/%.o: %.c | build
	$(GCC) $(GCCFLAGS) -I./include -I../include -c $< -o $@

build/%.o: asm/%.asm | build
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT): | build
	$(GCC) $(GCCFLAGS) -c $(LOADERSRC) -o $(LOADEROBJECT)

build:
	mkdir -p build

out:
	mkdir -p out

clean:
	rm -rf build out

.PHONY: clean
