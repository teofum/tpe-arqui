include Makefile.inc

# Make all targets in the docker container
# This is the default target for convenience, to build locally use "make all"
remote:
	docker start tpe-builder # Start the container if it's not running
	docker exec -it tpe-builder make all -C /root

# =============================================================================
# Toolchain
# =============================================================================

MP=Toolchain/ModulePacker/mp.bin
MP_SOURCES=$(wildcard Toolchain/ModulePacker/*.c)

$(MP): $(MP_SOURCES)
	gcc $(MP_SOURCES) -o $(MP)

# =============================================================================
# Kernel
# =============================================================================

KERNEL_DIR=Kernel
KERNEL=$(KERNEL_DIR)/out/kernel.bin
KERNEL_DEBUG_ELF=$(KERNEL:.bin=.elf)
KERNEL_SOURCES=$(wildcard $(KERNEL_DIR)/src/*.c)
KERNEL_SOURCES_ASM=$(wildcard $(KERNEL_DIR)/asm/*.asm)
KERNEL_OBJECTS=$(foreach I,$(notdir $(KERNEL_SOURCES:.c=.o)),$(KERNEL_DIR)/build/$I)
KERNEL_OBJECTS_ASM=$(foreach I,$(notdir $(KERNEL_SOURCES_ASM:.asm=.o)),$(KERNEL_DIR)/build/$I)
KERNEL_LOADERSRC=$(KERNEL_DIR)/loader.asm
KERNEL_LOADEROBJECT=$(foreach I,$(notdir $(KERNEL_LOADERSRC:.asm=.o)),$(KERNEL_DIR)/build/$I)

$(KERNEL): $(KERNEL_LOADEROBJECT) $(KERNEL_OBJECTS) $(KERNEL_STATICLIBS) $(KERNEL_OBJECTS_ASM) | $(KERNEL_DIR)/out
	$(LD) $(KERNEL_LDFLAGS) -T $(KERNEL_DIR)/kernel.ld -o $(KERNEL) $(KERNEL_LOADEROBJECT) $(KERNEL_OBJECTS) $(KERNEL_OBJECTS_ASM) $(KERNEL_STATICLIBS)

$(DEBUG_ELF): $(KERNEL_LOADEROBJECT) $(KERNEL_OBJECTS) $(KERNEL_STATICLIBS) $(KERNEL_OBJECTS_ASM) | $(KERNEL_DIR)/out
	$(LD) $(KERNEL_LDFLAGS) -T $(KERNEL_DIR)/kernel.ld --oformat=elf64-x86-64 -o $(KERNEL_DEBUG_ELF) $(KERNEL_LOADEROBJECT) $(KERNEL_OBJECTS) $(KERNEL_OBJECTS_ASM) $(KERNEL_STATICLIBS)

$(KERNEL_DIR)/build/%.o: $(KERNEL_DIR)/src/%.c | $(KERNEL_DIR)/build
	$(GCC) $(KERNEL_GCCFLAGS) -I$(KERNEL_DIR)/include -c $< -o $@

$(KERNEL_DIR)/build/%.o: $(KERNEL_DIR)/asm/%.asm | $(KERNEL_DIR)/build
	$(ASM) $(ASMFLAGS) $< -o $@

$(KERNEL_LOADEROBJECT): | $(KERNEL_DIR)/build
	$(ASM) $(ASMFLAGS) $(KERNEL_LOADERSRC) -o $(KERNEL_LOADEROBJECT)

$(KERNEL_DIR)/build:
	cd $(KERNEL_DIR); mkdir -p build

$(KERNEL_DIR)/out:
	cd $(KERNEL_DIR); mkdir -p out

# =============================================================================
# Userland
# =============================================================================

USERLAND_DIR=Userland
USERLAND=$(USERLAND_DIR)/out/userlandModule.bin
USERLAND_ASSETS=capibara.meme capybase.mdl capyface.mdl capyclub.mdl flag.mdl flagpole.mdl ball.mdl utah.mdl
USERLAND_ASSETS_PATH=$(foreach I,$(USERLAND_ASSETS),$(USERLAND_DIR)/assets/$I)

USER_DEBUG_ELF=$(USERLAND:.bin=.elf)
USER_SOURCES=$(wildcard $(USERLAND_DIR)/src/*.c)
USER_SOURCES_ASM=$(wildcard $(USERLAND_DIR)/asm/*.asm)
USER_OBJECTS=$(foreach I,$(notdir $(USER_SOURCES:.c=.o)),$(USERLAND_DIR)/build/$I)
USER_OBJECTS_ASM=$(foreach I,$(notdir $(USER_SOURCES_ASM:.asm=.o)),$(USERLAND_DIR)/build/$I)
USER_LOADERSRC=$(USERLAND_DIR)/_loader.c
USER_LOADEROBJECT=$(foreach I,$(notdir $(USER_LOADERSRC:.c=.o)),$(USERLAND_DIR)/build/$I)

$(USERLAND): $(USER_LOADEROBJECT) $(USER_OBJECTS) $(USER_STATICLIBS) $(USER_OBJECTS_ASM) | $(USERLAND_DIR)/out
	$(LD) $(LDFLAGS) -T $(USERLAND_DIR)/userlandModule.ld -o $(USERLAND) $(USER_LOADEROBJECT) $(USER_OBJECTS) $(USER_OBJECTS_ASM)

$(USER_DEBUG_ELF): $(USER_LOADEROBJECT) $(USER_OBJECTS) $(USER_STATICLIBS) $(USER_OBJECTS_ASM) | $(USERLAND_DIR)/out
	$(LD) $(LDFLAGS) -T $(USERLAND_DIR)/userlandModule.ld --oformat=elf64-x86-64 -o $(USER_DEBUG_ELF) $(USER_LOADEROBJECT) $(USER_OBJECTS) $(USER_OBJECTS_ASM)

$(USERLAND_DIR)/build/%.o: $(USERLAND_DIR)/src/%.c | $(USERLAND_DIR)/build
	$(GCC) $(GCCFLAGS) -I$(USERLAND_DIR)/include -c $< -o $@

$(USERLAND_DIR)/build/%.o: $(USERLAND_DIR)/asm/%.asm | $(USERLAND_DIR)/build
	$(ASM) $(ASMFLAGS) $< -o $@

$(USER_LOADEROBJECT): | $(USERLAND_DIR)/build
	$(GCC) $(GCCFLAGS) -c $(USER_LOADERSRC) -o $(USER_LOADEROBJECT)

$(USERLAND_DIR)/build:
	cd $(USERLAND_DIR); mkdir -p build

$(USERLAND_DIR)/out:
	cd $(USERLAND_DIR); mkdir -p out

# =============================================================================
# Bootloader
# =============================================================================

BOOTLOADER_DIR=Bootloader
BMFS=$(BOOTLOADER_DIR)/out/bmfs.bin
MBR=$(BOOTLOADER_DIR)/out/bmfs_mbr.sys
PURE64=$(BOOTLOADER_DIR)/out/pure64.sys

$(BMFS): | $(BOOTLOADER_DIR)/out
	gcc -ansi -std=c99 -Wall -pedantic -o $(BMFS) $(BOOTLOADER_DIR)/BMFS/bmfs.c

$(MBR): | $(BOOTLOADER_DIR)/out
	cd $(BOOTLOADER_DIR)/Pure64/src; $(ASM) bootsectors/bmfs_mbr.asm -o ../../../$(MBR)

$(PURE64): | $(BOOTLOADER_DIR)/out
	cd $(BOOTLOADER_DIR)/Pure64/src; $(ASM) pure64.asm -o ../../../$(PURE64)

$(BOOTLOADER_DIR)/out:
	cd $(BOOTLOADER_DIR); mkdir -p out

# =============================================================================
# Final output
# =============================================================================

# Output
OSIMAGENAME=out/x64BareBonesImage
VMDK=$(OSIMAGENAME).vmdk
QCOW2=$(OSIMAGENAME).qcow2
IMG=$(OSIMAGENAME).img
PACKEDKERNEL=out/packedKernel.bin
IMGSIZE=16777216

all: $(IMG) $(VMDK) $(QCOW2)

$(IMG): $(BMFS) $(MBR) $(PURE64) $(PACKEDKERNEL) | out
	$(BMFS) $(IMG) initialize $(IMGSIZE) $(MBR) $(PURE64) $(PACKEDKERNEL)

$(VMDK): $(IMG) | out
	qemu-img convert -f raw -O vmdk $(IMG) $(VMDK)

$(QCOW2): $(IMG) | out
	qemu-img convert -f raw -O qcow2 $(IMG) $(QCOW2)

$(PACKEDKERNEL): $(KERNEL) $(USERLAND) $(USERLAND_ASSETS_PATH) $(MP) | out
	$(MP) $(KERNEL) $(USERLAND) $(USERLAND_ASSETS_PATH) -o $(PACKEDKERNEL)

# =============================================================================
# Utils
# =============================================================================

out:
	mkdir -p out

container:
	docker pull agodio/itba-so:2.0
	docker run -d -v $(PWD):/root --security-opt seccomp:unconfined -it --name tpe-builder --platform=linux/amd64 agodio/itba-so:2.0

run:
	./run.sh

debug:
	./run.sh -d

clean:
	rm -rf Kernel/out
	rm -rf Kernel/build
	rm -rf Userland/out
	rm -rf Userland/build
	rm -rf Bootloader/out
	rm -f Toolchain/ModulePacker/mp.bin
	rm -rf out

.PHONY: remote container clean
