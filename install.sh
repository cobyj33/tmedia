#!/bin/bash

if [ ! -d ~/.local/bin ]
then
    mkdir ~/.local/bin
    echo "export $PATH=$HOME/.local/bin:$PATH" >> ".$(echo $SHELL | sed -nE "s|.*/(.*)\$|\1|p")rc"
fi

if [ -e "./build/ascii_video" ]
then
    cp -R ./build/ascii_video ~/.local/bin
    hash -r
else
    echo "Cannot install ascii_video binary, as the ascii_video binary does not currently exist. Run 'chmod +x ./build.sh && ./build.sh' to build the binary"
fi
