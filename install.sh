#!/bin/bash
# set -xe

project_root=$(pwd)

if [ ! -d "${project_root}/lib/ncurses-6.4" ] 
then
    tar -xf ${project_root}/lib/ncurses-6.4.tar.gz -C ${project_root}/lib
    cd ${project_root}/lib/ncurses-6.4 # in ncurses project
    mkdir ${project_root}/lib/ncurses-6.4/build 
    mkdir ${project_root}/lib/ncurses-6.4/build/data
    ./configure --prefix=${project_root}/lib/ncurses-6.4/build --datadir=${project_root}/lib/ncurses-6.4/build/data
    make -j4
    make install
    cd ${project_root}
fi

# if [ ! -d "${project_root}/lib/ffmpeg-5.1.2" ] 
# then
#     sudo apt-get update -qq

#     sudo apt-get -y install \
#     autoconf \
#     automake \
#     build-essential \
#     cmake \
#     git-core \
#     libass-dev \
#     libfreetype6-dev \
#     libgnutls28-dev \
#     libmp3lame-dev \
#     libsdl2-dev \
#     libtool \
#     libva-dev \
#     libvdpau-dev \
#     libvorbis-dev \
#     libxcb1-dev \
#     libxcb-shm0-dev \
#     libxcb-xfixes0-dev \
#     meson \
#     ninja-build \
#     pkg-config \
#     texinfo \
#     wget \
#     yasm \
#     zlib1g-dev \
#     libunistring-dev \
#     libaom-dev \
#     libdav1d-dev

#     tar -xf ${project_root}/lib/ffmpeg-5.1.2.tar.xz -C ${project_root}/lib
#     ffmpeg_dir=${project_root}/lib/ffm
#     ffmpeg_build=${ffmpeg_dir}/ffmpeg-5.1.2
#     ffmpeg_bin=${ffmpeg_dir}/binpeg_build

#     mkdir ${ffmpeg_bin}
#     mkdir ${ffmpeg_build}

#     cd ${ffmpeg_dir}

#     PATH="${ffmpeg_bin}:$PATH"
#     PKG_CONFIG_PATH="${ffmpeg_build}/lib/pkgconfig"
#     ./configure \
#     --prefix="${ffmpeg_build}" \
#     --pkg-config-flags="--static" \
#     --extra-cflags="-I${ffmpeg_build}/include" \
#     --extra-ldflags="-L${ffmpeg_build}/lib" \
#     --extra-libs="-lpthread -lm" \
#     --ld="g++" \
#     --bindir="${ffmpeg_bin}" \
#     --enable-gpl \
#     --enable-gnutls \
#     --enable-libaom \
#     --enable-libass \
#     --enable-libfreetype \
#     --enable-libmp3lame \
#     --enable-libdav1d \
#     --enable-libvorbis \
#     --enable-nonfree \

#     PATH="${ffmpeg_dir}/bin:$PATH" 


#     make -j4
#     make install
#     hash -r
#     sudo apt-get autoremove
# fi

