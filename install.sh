#!/bin/bash
# set -xe

SUDO=''
if (( $EUID != 0 )); then
    SUDO='sudo'
fi

project_root=$(pwd)
project_lib_folder=${project_root}/lib
lib_build=${project_lib_folder}/build

ncurses_tar=${project_lib_folder}/ncurses-6.4.tar.gz
ncurses_src=${project_lib_folder}/ncurses-6.4
ncurses_build=${lib_build}/ncurses-6.4
ncurses_datadir=${ncurses_build}/data


ffmpeg_tar=${project_lib_folder}/ffmpeg-6.0.tar.xz
ffmpeg_src=${project_lib_folder}/ffmpeg-6.0
ffmpeg_build=${lib_build}/ffmpeg-6.0

ascii_video_build=${project_root}/build

if [ ! -d ${lib_build} ]
then
    mkdir ${lib_build}
fi

if [ ! -d ${ncurses_build} ] 
then
    if [ ! -d ${ncurses_src} ]
    then
        tar -xvf ${ncurses_tar} -C ${project_lib_folder}
    fi

    cd ${ncurses_src}

    # mkdir ${project_lib_folder}/ncurses-6.4/build 
    # mkdir ${project_lib_folder}/ncurses-6.4/build/data
    if [ ! -d ${ncurses_build} ]
    then
        mkdir ${ncurses_build}
    fi

    if [ ! -d ${ncurses_datadir} ]
    then
        mkdir ${ncurses_datadir}
    fi

    ./configure \
    --prefix=${ncurses_build} \
    --datadir=${ncurses_datadir} \
    --without-tests \
    --without-manpages \
    --without-progs


    make -j4
    make install
    cd ${project_root}
    rm -rf ${ncurses_src}
fi

if [ ! -d ${ffmpeg_build} ] 
then

    mkdir ${ffmpeg_build}
    if [ ! -d ${ffmpeg_src} ]
    then
        tar -xvf ${ffmpeg_tar} -C ${project_lib_folder}
    fi
    cd ${ffmpeg_src}

    $SUDO apt-get update -qq

    deps="autoconf \
    automake \
    build-essential \
    cmake \
    git \
    libtool \
    meson \
    ninja-build \
    pkg-config \
    yasm \
    libva-dev \
    libvdpau-dev \
    libvorbis-dev \
    libxcb1-dev \
    libxcb-shm0-dev \
    libxcb-xfixes0-dev \
    libunistring-dev \
    libaom-dev \
    libdav1d-dev"

    $SUDO apt-get install -y \
    autoconf \
    automake \
    build-essential \
    cmake \
    git \
    libtool \
    meson \
    ninja-build \
    pkg-config \
    yasm \
    libva-dev \
    libvdpau-dev \
    libvorbis-dev \
    libxcb1-dev \
    libxcb-shm0-dev \
    libxcb-xfixes0-dev \
    libunistring-dev \
    libaom-dev \
    libdav1d-dev


    PKG_CONFIG_PATH="${ffmpeg_build}/lib/pkgconfig"
    ./configure \
    --prefix="${ffmpeg_build}" \
    --pkg-config-flags="--static" \
    --extra-cflags="-I${ffmpeg_build}/include" \
    --extra-ldflags="-L${ffmpeg_build}/lib" \
    --extra-libs="-lpthread -lm" \
    --ld="g++" \
    --enable-gpl \
    --enable-nonfree \
    --enable-libaom \
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
    --enable-protocol=file,pipe \
    --disable-devices

    make -j4
    make install
    hash -r
    rm -rf ${ffmpeg_src}
    cd ${project_root}
fi

if [ -d ${ascii_video_build} ]
then
    rm -rf ${ascii_video_build}
fi

mkdir ${ascii_video_build}
cd ${ascii_video_build}
cmake ../
make -j4




    