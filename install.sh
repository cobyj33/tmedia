#!/bin/bash
set -xe

project_root=$(pwd)

get_local_bin_path_addition_string() {
    echo "# set PATH so it includes user's private bin if it exists"
    echo "if [ -d \"$1\" ] ; then"
    echo "  PATH=\"$1:\$PATH\""
    echo "fi"
}

prompt_confirm() {
    while true; do
        read -r -n 1 -p "${1:-Continue?} [y/n]: " REPLY
        echo ""
        case $REPLY in
        [yY]) echo ; return 0 ;;
        [nN]) echo ; return 1 ;;
        *) printf " \033[31m %s \n\033[0m" "invalid input"
        esac 
    done  
}

add_local_bin_path() {
    if [[ -f $2 ]]
    then
        grep PATH=\"$1:\$PATH\" $2
        if [[ ! $? = "" ]]
        then
            echo "Found that $1 has already been appended to the path variable. No write done to $2"
            return 1
        else
            echo "Adding $1 to path declarations in file $2"
            get_local_bin_path_addition_string $1 | cat >> $2
            return 0
        fi
    else
        echo "Failure: Tried to add $1 to PATH in file \"$2\" . File does not exist"
        return 2
    fi
}

previously_installed_location="$(whereis ascii_video | sed "s/ascii_video:[ ]*//")"

if [[ ! $previously_installed_location = "" ]]
then
    prompt_confirm "ascii_video detected to be installed at $previously_installed_location. Would you like to overwrite?"
    if [[ $? -eq 0 ]] 
    then
        echo "Set to overwrite ascii_video installation at $previously_installed_location"
    else
        echo "Will not overwrite ascii_video installation at $previously_installed_location."
        echo "Exiting installation script for ascii_video"
        exit 0
    fi
fi

if [[ ! -f $project_root/build/ascii_video ]]
then
    echo "Cannot install ascii_video binary, as the ascii_video binary does not currently exist. Run 'chmod +x ./build.sh && ./build.sh' to build the binary"
    exit 1
fi

global_install_location="/usr/local/bin"
user_install_location="$HOME/.local/bin"


[[ $EUID -eq 0 ]]
is_root=$?
install_location=$([[ is_root -eq 0 ]] && echo $global_install_location || echo $user_install_location)

if [[ ! is_root -eq 0 ]]
then
    install_location=$user_install_location

    if [[ ! -d $user_install_location ]]
    then
        echo "$user_install_location could not be found. Creating $user_install_location"
        mkdir -p $user_install_location
    fi

    shell_name=$( echo $SHELL | sed -nE "s|.*/(.*)\$|\1|p" )
    shell_rc="$HOME/.${shell_name}rc"

    if [[ -f $shell_rc ]]
    then
        add_local_bin_path $user_install_location $shell_rc
    elif [[ -f $HOME/.bash_profile ]]
    then
        add_local_bin_path $user_install_location $HOME/.bash_profile
    elif [[ -f $HOME/.bash_login ]]
    then
        add_local_bin_path $user_install_location $HOME/.bash_login
    elif [[ -f $HOME/.profile ]]
    then
        add_local_bin_path $user_install_location $HOME/.profile
    else
        echo "Could not add $user_install_location, could not find proper shell configuration file to modify path"
    fi

fi

if [[ -f $project_root/build/ascii_video ]]
then
    echo "Installing ascii_video binary to $install_location"
    cp -R $project_root/build/ascii_video $install_location
    hash -r
else
    echo "Cannot install ascii_video binary, as the ascii_video binary does not currently exist. Run 'chmod +x ./build.sh && ./build.sh' to build the binary"
fi
