#!/bin/bash
# set -xe

SUDO=''
if (( $EUID != 0 )); then
    SUDO='sudo'
fi

$SUDO apt-get update
$SUDO apt-get install git build-essential cmake pkg-config libavdevice-dev libncurses-dev

git submodule update --init

mkdir build
cd build
cmake ../
make -j