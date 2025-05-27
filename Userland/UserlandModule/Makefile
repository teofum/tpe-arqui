include ../Makefile.inc

MODULE=0000-userlandModule.bin
SOURCES=$(wildcard [^_]*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(foreach I,$(SOURCES:.c=.o),build/$I)
OBJECTS_ASM=$(foreach I,$(notdir $(SOURCES_ASM:.asm=.o)),build/$I)
LOADERSRC=_loader.c

LOADEROBJECT=$(foreach I,$(LOADERSRC:.c=.o),build/$I)

all: $(MODULE)

$(MODULE): out $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
	$(LD) $(LDFLAGS) -T userlandModule.ld -o ../$(MODULE) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM)

build/%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -I../include -c $< -o $@

build/%.o: asm/%.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT): build
	$(GCC) $(GCCFLAGS) -c $(LOADERSRC) -o $(LOADEROBJECT)

build:
	mkdir build

out:
	mkdir out

clean:
	rm -rf build out asm/*.o *.o *.bin

.PHONY: all clean print
