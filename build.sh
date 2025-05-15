#!/bin/bash

# Start the container if it's not running
docker start tpe-builder

# Clean and rebuild project
docker exec -it tpe-builder make clean -C /root/Toolchain
docker exec -it tpe-builder make clean -C /root
docker exec -it tpe-builder make -C /root/Toolchain
docker exec -it tpe-builder make -C /root
