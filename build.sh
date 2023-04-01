#!/bin/bash

# set -xe

project_name="ascii_video"
author="cobyj33"
version="0.4.0"
github_url="https://www.github.com/cobyj33/ascii_video"
git_repo="https://www.github.com/cobyj33/ascii_video.git"

libavcodec_min_version="59.42.100"
libavformat_min_version="59.30.100"
libavutil_min_version="57.32.101"
libswresample_min_version="4.8.100"
libswscale_min_version="6.8.102"

is_debian=$([[ -f "/etc/debian_version" ]])

n_proc=$(nproc --all)
make_proc=1
if [[ n_proc -gt 1 ]]
then
    make_proc=$(($n_proc / 2))
fi

if [[ ! $is_debian -eq 0 ]]; then
   echo "Cannot build $project_name. Project can only currently build \ 
   and is only currently tested to build on Debian-Linux systems. This is in place \
   to protect any errors that it would cause on untested operating systems."
   exit 1
fi

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

is_ncurses6_already_installed_globally() {
    [[ ! $(ldconfig -p | grep "libncurses.so.6\|libncursesw.so.6") = \"\"  ]]
    return $?
}

echo "Building $project_name by $author. Version $version. See source code at $github_url"

# Use the sudo command if the user is not root
SUDO=''
if [[ ! $EUID -eq 0 ]]
then
    SUDO='sudo'
fi

# Basic dependencies needed to build software on Debian-Type distributions 
# These will be needed for ncurses and building ascii_video itself
building_deps="git \
autoconf \
automake \
build-essential \
make \
cmake \
git \
libtool \
pkg-config"

ffmpeg_deps="meson \
ninja-build \
yasm \
libva-dev \
libvorbis-dev \
libunistring-dev \
libdav1d-dev"

ncurses_apt="libncurses6 \
libncurses-dev"

apt_deps_to_install="$building_deps $ncurses_apt"

if [[ ! -d $(pwd)/lib/build/ffmpeg-6.0 ]] #Check to see if ffmpeg 6-0 has been compiled
then
    apt_deps_to_install="$apt_deps_to_install $ffmpeg_deps"
fi

if [[ ! "$apt_deps_to_install" = "" ]]
then
    $SUDO apt-get update
    $SUDO apt-get install $apt_deps_to_install
fi

project_root=$(pwd)
project_lib_folder=$project_root/lib
lib_build=$project_lib_folder/build

ncurses_tar=$project_lib_folder/ncurses-6.4.tar.gz
ncurses_src=$project_lib_folder/ncurses-6.4
ncurses_build=$lib_build/ncurses-6.4
ncurses_datadir=$ncurses_build/data


ffmpeg_tar=$project_lib_folder/ffmpeg-6.0.tar.xz
ffmpeg_src=$project_lib_folder/ffmpeg-6.0
ffmpeg_build=$lib_build/ffmpeg-6.0

ascii_video_build=$project_root/build

mkdir -p $lib_build

# Defaults to local ncurses build if that is found
should_install_ncurses6_locally=true

if [[ $(is_ncurses6_already_installed_globally) -eq 0 ]]
then

    if [[ -d $ncurses_build ]]
    then
        echo "Found a working ncurses6 installation already compiled locally and a globally installed ncurses6 lib."
    else # Ncurses build not found
        echo "Did not find a working ncurses6 installation compiled locally, but did find a globally installed ncurses6 lib."
    fi

    echo "Defaulting to link toward global ncurses6 installation"
    should_install_ncurses6_locally=false # Already installed globally, cmake will link toward the global build
else 
    echo "Could not find a global installation of libncurses6"

    if [[ is_debian -eq 0 ]]
    then
        prompt_confirm "Would you like to install ncurses6 with apt installer"
        if [[ $? -eq 0 ]] 
        then
            echo "Installing ncurses apt-packages \"$ncurses_apt\" with apt package manager"
            $SUDO apt-get install $ncurses_apt 
            echo "Installed ncurses apt-packages \"$ncurses_apt\" with apt package manager"
            echo "Defaulting to link toward global ncurses6 installation"
            should_install_ncurses6_locally=false
        else
            echo "Defaulting to install ncurses6 in repository lib/build folder"
            should_install_ncurses6_locally=true
        fi
    fi

fi

if [[ should_install_ncurses6_locally = true ]]
then
    should_compile=true

    if [[ -d $ncurses_build ]]
    then
        prompt_confirm "Found local ncurses6 build at $ncurses_build. Recompile the library?"
        [[ $? -eq 0 ]] && should_compile=true || should_compile=false
    fi


    if [[ should_compile = true ]]
    then
        echo "Creating local installation of ncurses6 for ascii_video"
        rm -rf $ncurses_src $ncurses_build
        tar -xvf $ncurses_tar -C $project_lib_folder
        cd $ncurses_src
        mkdir -p $ncurses_build
        mkdir -p $ncurses_datadir

        ./configure \
        --prefix=$ncurses_build \
        --datadir=$ncurses_datadir \
        --without-tests \
        --without-manpages \
        --without-progs


        make -j$make_proc
        make install
        cd $project_root
        rm -rf $ncurses_src
    else
        echo "Aborting creation of local installation of ncurses6 for ascii_video. ncurses6 already installed locally at lib/build/ncurses-6.4"
    fi

fi


if [[ ! -d $ffmpeg_build ]]
then
    mkdir -p $ffmpeg_build
    rm -rf $ffmpeg_src
    tar -xvf $ffmpeg_tar -C $project_lib_folder
    cd $ffmpeg_src

    PKG_CONFIG_PATH="$ffmpeg_build/lib/pkgconfig"

    ./configure \
    --prefix="$ffmpeg_build" \
    --pkg-config-flags="--static" \
    --extra-cflags="-I$ffmpeg_build/include" \
    --extra-ldflags="-L$ffmpeg_build/lib" \
    --extra-libs="-lpthread -lm" \
    --ld="g++" \
    --enable-gpl \
    --enable-nonfree \
    --enable-libdav1d \
    --enable-libvorbis \
    --disable-network \
    --disable-encoders \
    --disable-muxers \
    --enable-static \
    --disable-shared \
    --disable-programs \
    --disable-doc \
    --disable-filters \
    --disable-postproc \
    --disable-avfilter \
    --disable-avdevice \
    --disable-protocols \
    --disable-lzma \
    --enable-protocol=file,pipe \
    --disable-devices \
    --disable-autodetect \
    --disable-iconv

    make -j$make_proc
    make install
    hash -r
    rm -rf $ffmpeg_src
    cd $project_root
fi

rm -rf $ascii_video_build
mkdir $ascii_video_build
cd $ascii_video_build
cmake ../
make -j$make_proc
