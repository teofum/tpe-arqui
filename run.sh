#!/bin/bash

audio_driver=pa
if [[ "$OSTYPE" == "darwin"* ]]; then
    audio_driver=coreaudio
fi

if [ "$1" = "-d" ]; then
    qemu-system-x86_64 -hda out/x64BareBonesImage.qcow2 -m 512 -audiodev $audio_driver,id=speaker -machine pcspk-audiodev=speaker -cpu max -S -s -d int 2>&1 | grep "v=" | grep -v "v=20"
elif [ "$1" = "-v" ]; then
    qemu-system-x86_64 -hda out/x64BareBonesImage.qcow2 -m 512 -audiodev $audio_driver,id=speaker -machine pcspk-audiodev=speaker -cpu max -d int 2>&1 | grep "v=" | grep -v "v=20"
else
    qemu-system-x86_64 -hda out/x64BareBonesImage.qcow2 -m 512 -audiodev $audio_driver,id=speaker -machine pcspk-audiodev=speaker -cpu max
fi
