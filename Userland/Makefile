include Makefile.inc

SAMPLE_DATA=0001-imageData.bin

all: userlandModule sampleDataModule

userlandModule:
	cd UserlandModule; make

sampleDataModule:
	cp capibara.meme $(SAMPLE_DATA)

clean:
	cd UserlandModule; make clean
	rm -rf *.bin


.PHONY: userlandModule all clean
