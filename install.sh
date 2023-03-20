#!/bin/bash
set -xe

project_root=$(pwd)

tar -xz ./lib/ncurses-6.4.tar.gz -C ${project_root}/lib
cd ${project_root}/lib/ncurses-6.4 # in ncurses project
mkdir ${project_root}/lib/ncurses-6.4/build 
mkdir ${project_root}/lib/ncurses-6.4/build/data
./configure --prefix=${project_root}/lib/ncurses-6.4/build --datadir=${project_root}/lib/ncurses-6.4/build/data
make -j4
make install

cd project_root

# tar -xz ./lib/ffmpeg-5.1.2.tar.xz -C ${project_root}/lib

if [ ! -d "./build" ] 
then
    echo "Creating build directory"
    mkdir ${project_root}/build
fi

mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
make -j4
