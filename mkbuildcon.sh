#!/bin/bash

docker pull agodio/itba-so:2.0
docker run -d -v $PWD:/root --security-opt seccomp:unconfined -it --name tpe-builder --platform=linux/amd64 agodio/itba-so:2.0
