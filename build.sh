#!/bin/bash
project_root=$(pwd)

if [ -d "$(pwd)/build" ] 
then
    echo "Clearing old Build Directory"
    rmdir $(pwd)/build
fi

echo "Creating build directory"
mkdir $(pwd)/build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
make -j4
