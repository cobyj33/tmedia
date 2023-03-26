#!/bin/bash
# set -xe

SUDO=''
if (( $EUID != 0 )); then
    SUDO='sudo'
fi

project_root=$(pwd)
project_lib_folder=${project_root}/lib
lib_bin=${project_lib_folder}/bin

ncurses_tar=${project_lib_folder}/ncurses-6.4.tar.gz
ncurses_src=${project_lib_folder}/ncurses-6.4
ncurses_bin=${lib_bin}/ncurses-6.4
ncurses_datadir=${ncurses_bin}/data


ffmpeg_tar=${project_lib_folder}/ffmpeg-6.0.tar.xz
ffmpeg_src=${project_lib_folder}/ffmpeg-6.0
ffmpeg_bin=${lib_bin}/ffmpeg-6.0

ascii_video_build=${project_root}/build

if [ ! -d ${lib_bin} ]
then
    mkdir ${lib_bin}
fi

if [ ! -d ${ncurses_bin} ] 
then
    if [ ! -d ${ncurses_src} ]
    then
        tar -xvf ${ncurses_tar} -C ${project_lib_folder}
    fi

    cd ${ncurses_src}

    # mkdir ${project_lib_folder}/ncurses-6.4/build 
    # mkdir ${project_lib_folder}/ncurses-6.4/build/data
    if [ ! -d ${ncurses_bin} ]
    then
        mkdir ${ncurses_bin}
    fi

    if [ ! -d ${ncurses_datadir} ]
    then
        mkdir ${ncurses_datadir}
    fi

    ./configure \
    --prefix=${ncurses_bin} \
    --datadir=${ncurses_datadir}

    make -j4
    make install
    cd ${project_root}
    rm -rf ${ncurses_src}
fi

if [ ! -d ${ffmpeg_bin} ] 
then

    mkdir ${ffmpeg_bin}
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


    PKG_CONFIG_PATH="${ffmpeg_bin}/lib/pkgconfig"
    ./configure \
    --prefix="${ffmpeg_bin}" \
    --pkg-config-flags="--static" \
    --extra-cflags="-I${ffmpeg_bin}/include" \
    --extra-ldflags="-L${ffmpeg_bin}/lib" \
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




    