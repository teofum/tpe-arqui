sudo docker start CONT
sudo docker exec -it CONT make clean -C /root/Toolchain
sudo docker exec -it CONT make clean -C /root/
sudo docker exec -it CONT make -C /root/Toolchain
sudo docker exec -it CONT make -C /root/

sudo qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512

#sudo docker stop CONT

