#!/bin/bash

audio_driver=pa
if [[ "$OSTYPE" == "darwin"* ]]; then
    audio_driver=coreaudio
fi

if [ "$1" = "-d" ]; then
    qemu-system-x86_64 -hda out/x64BareBonesImage.qcow2 -m 512 -audiodev $audio_driver,id=speaker -machine pcspk-audiodev=speaker -cpu max -S -s
else
    qemu-system-x86_64 -hda out/x64BareBonesImage.qcow2 -m 512 -audiodev $audio_driver,id=speaker -machine pcspk-audiodev=speaker -cpu max
fi
