#!/bin/bash

if [ ! -f "./build/ascii_video" ]
then
    echo "Cannot install ascii_video binary, as the ascii_video binary does not currently exist. Run 'chmod +x ./build.sh && ./build.sh' to build the binary"
    exit 1
fi

is_root=true
install_location="~/.local/bin"

if (( $EUID != 0 )); then
    is_root=false
    install_location="/usr/local/bin"
fi

if [ is_root -eq false ]
then

    if [ ! -d ~/.local/bin ]
    then
        echo "~/.local could not be found. Creating ~/.local"
        mkdir ~/.local
    fi

    if [ ! -d ~/.local/bin ]
    then
        echo "~/.local/bin could not be found. Creating ~/.local/bin"
        mkdir ~/.local/bin
        shell_rc=${"~/.$(echo $SHELL | sed -nE "s|.*/(.*)\$|\1|p")rc"}

        if [ ! -e ${shell_rc}]
        then   
            echo "Could not add ~/.local/bin to path, could not find shell rc file to modify to modify path"
        else
            echo "Adding ~/.local/bin to ${shell_rc}"
            echo "export $PATH=$HOME/.local/bin:$PATH" >> ${shell_rc}
        fi
    fi

fi


if [ -f "./build/ascii_video" ]
then
    echo "Installing ascii_video binary to ${install_location}"
    cp -R ./build/ascii_video ${install_location}
    hash -r
else
    echo "Cannot install ascii_video binary, as the ascii_video binary does not currently exist. Run 'chmod +x ./build.sh && ./build.sh' to build the binary"
fi
