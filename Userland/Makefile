include Makefile.inc

all: out/userlandModule.bin

out/userlandModule.bin: | out
	make -C UserlandModule
	ln UserlandModule/out/userlandModule.bin out/userlandModule.bin

out:
	mkdir -p out

clean:
	make clean -C UserlandModule
	rm -rf out

.PHONY: clean
