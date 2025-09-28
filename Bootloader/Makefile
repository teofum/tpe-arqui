all: out/bmfs.bin out/pure64.sys out/bmfs_mbr.sys out/pxestart.sys

out/bmfs.bin: | out
	make -C BMFS
	ln BMFS/bmfs.bin out/bmfs.bin

out/pure64.sys: | out
	make pure64.sys -C Pure64
	rm -f out/pure64.sys
	ln -f Pure64/pure64.sys out/pure64.sys

out/bmfs_mbr.sys: | out
	make bmfs_mbr.sys -C Pure64
	rm -f out/bmfs_mbr.sys
	ln -f Pure64/bmfs_mbr.sys out/bmfs_mbr.sys

out/pxestart.sys: | out
	make pxestart.sys -C Pure64
	rm -f out/pxestart.sys
	ln -f Pure64/pxestart.sys out/pxestart.sys

out:
	mkdir -p out

clean:
	make clean -C BMFS
	make clean -C Pure64
	rm -rf out

.PHONY: pure64 clean
