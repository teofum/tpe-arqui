# Module packer utility
MP=Toolchain/ModulePacker/mp.bin

# Bootloader
BOOTLOADER_PATH=Bootloader
BMFS=$(BOOTLOADER_PATH)/out/bmfs.bin
MBR=$(BOOTLOADER_PATH)/out/bmfs_mbr.sys
PURE64=$(BOOTLOADER_PATH)/out/pure64.sys

# Kernel
KERNEL=Kernel/out/kernel.bin

# Userland
USERLAND=Userland/out/userlandModule.bin
USERLAND_ASSETS=capibara.meme capybase.mdl capyface.mdl capyclub.mdl flag.mdl flagpole.mdl ball.mdl utah.mdl
USERLAND_ASSETS_PATH=$(foreach I,$(USERLAND_ASSETS),Userland/assets/$I)

# Output
OSIMAGENAME=out/x64BareBonesImage
VMDK=$(OSIMAGENAME).vmdk
QCOW2=$(OSIMAGENAME).qcow2
IMG=$(OSIMAGENAME).img
PACKEDKERNEL=out/packedKernel.bin
IMGSIZE=16777216

# Make all targets in the docker container
# This is the default target for convenience, to build locally use "make all"
remote:
	docker start tpe-builder # Start the container if it's not running
	docker exec -it tpe-builder make all -C /root

all: $(IMG) $(VMDK) $(QCOW2)

$(KERNEL):
	make -C Kernel

$(USERLAND):
	make -C Userland

$(BMFS):
	make out/bmfs.bin -C Bootloader

$(MBR):
	make out/bmfs_mbr.sys -C Bootloader

$(PURE64):
	make out/pure64.sys -C Bootloader

$(MP):
	make -C Toolchain

$(PACKEDKERNEL): $(KERNEL) $(USERLAND) $(USERLAND_ASSETS_PATH) $(MP) | out
	$(MP) $(KERNEL) $(USERLAND) $(USERLAND_ASSETS_PATH) -o $(PACKEDKERNEL)

$(IMG): $(BMFS) $(MBR) $(PURE64) $(PACKEDKERNEL) | out
	$(BMFS) $(IMG) initialize $(IMGSIZE) $(MBR) $(PURE64) $(PACKEDKERNEL)

$(VMDK): $(IMG) | out
	qemu-img convert -f raw -O vmdk $(IMG) $(VMDK)

$(QCOW2): $(IMG) | out
	qemu-img convert -f raw -O qcow2 $(IMG) $(QCOW2)

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
	make clean -C Kernel
	make clean -C Userland
	make clean -C Bootloader
	make clean -C Toolchain
	rm -rf out

.PHONY: remote container clean
