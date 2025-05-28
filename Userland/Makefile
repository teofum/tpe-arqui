include Makefile.inc

SAMPLE_DATA=0001-sampleDataModule.bin

all: userlandModule sampleDataModule

userlandModule:
	cd UserlandModule; make

sampleDataModule:
	printf "This is sample data." >> $(SAMPLE_DATA) && dd if=/dev/zero bs=1 count=1 >> $(SAMPLE_DATA)

clean:
	cd UserlandModule; make clean
	rm -rf *.bin


.PHONY: userlandModule all clean
