BOOTLOADER_PATH=../Bootloader
BMFS=$(BOOTLOADER_PATH)/BMFS/bmfs.bin
MBR=$(BOOTLOADER_PATH)/Pure64/bmfs_mbr.sys
MP=../Toolchain/ModulePacker/mp.bin
PURE64=$(BOOTLOADER_PATH)/Pure64/pure64.sys
OSIMAGENAME=x64BareBonesImage
VMDK=$(OSIMAGENAME).vmdk
QCOW2=$(OSIMAGENAME).qcow2
IMG=$(OSIMAGENAME).img
KERNEL=../Kernel/out/kernel.bin
USERLAND=../Userland/0000-userlandModule.bin ../Userland/assets/capibara.meme ../Userland/assets/capybase.mdl ../Userland/assets/capyface.mdl ../Userland/assets/capyclub.mdl ../Userland/assets/flag.mdl ../Userland/assets/flagpole.mdl ../Userland/assets/ball.mdl ../Userland/assets/utah.mdl

PACKEDKERNEL=packedKernel.bin
IMGSIZE=16777216

all: $(IMG) $(VMDK) $(QCOW2)

$(KERNEL):
	cd ../Kernel; make

$(PACKEDKERNEL): $(KERNEL) $(USERLAND)
	$(MP) $(KERNEL) $(USERLAND) -o $(PACKEDKERNEL)

$(IMG): $(BMFS) $(MBR) $(PURE64) $(PACKEDKERNEL)
	$(BMFS) $(IMG) initialize $(IMGSIZE) $(MBR) $(PURE64) $(PACKEDKERNEL) 

$(VMDK): $(IMG)
	qemu-img convert -f raw -O vmdk $(IMG) $(VMDK) 

$(QCOW2): $(IMG)
	qemu-img convert -f raw -O qcow2 $(IMG) $(QCOW2)

clean:
	rm -rf $(IMG) $(VMDK) $(QCOW2) *.bin

.PHONY: all clean
