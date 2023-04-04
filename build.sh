#!/bin/bash

set -eu

# libdav1d: meson .. --default-library=static -Denable_docs=false -Dtestdata_tests=false -Denable_tools=false -Denable_examples=false -Denable_tests=false
# libvorbis: 

PROJECT_NAME="ascii_video"
AUTHOR="cobyj33"
VERSION="0.4.0"
GITHUB_URL="https://www.github.com/cobyj33/ascii_video"
GIT_REPO="https://www.github.com/cobyj33/ascii_video.git"

LIBAVCODEC_MIN_VERSION_MAJOR=58
LIBAVFORMAT_MIN_VERSION_MAJOR=58
LIBAVUTIL_MIN_VERSION_MAJOR=56
LIBSWRESAMPLE_MIN_VERSION_MAJOR=3
LIBSWSCALE_MIN_VERSION_MAJOR=5

check_pwd_is_repo() {
    CWD=$(pwd)
    if [[ -d $CWD/lib ]] && [[ -d $CWD/src ]] && [[ -d $CWD/include ]] && [[ -d $CWD/assets ]]
    then
        return 0
    else
        return 1
    fi
}

if ! check_pwd_is_repo;
then
    echo "Build script must be started from where the current working directory is the root of the $PROJECT_NAME repository."
    echo "please change directories to the $PROJECT_NAME repository root before running the script again"
    exit 1
fi

PROJECT_ROOT=$(pwd)
PROJECT_LIBDIR=$PROJECT_ROOT/lib
PROJECT_LIBDIR_BUILD=$PROJECT_LIBDIR/build

NCURSES_TAR=$PROJECT_LIBDIR/ncurses-6.4.tar.gz
NCURSES_SRC=$PROJECT_LIBDIR/ncurses-6.4
NCURSES_BUILD=$PROJECT_LIBDIR_BUILD/ncurses-6.4
NCURSES_DATADIR=$NCURSES_BUILD/data


FFMPEG_TAR=$PROJECT_LIBDIR/ffmpeg-6.0.tar.xz
FFMPEG_SRC=$PROJECT_LIBDIR/ffmpeg-6.0
FFMPEG_BUILD=$PROJECT_LIBDIR_BUILD/ffmpeg-6.0

LIBVORBIS_TAR=$PROJECT_LIBDIR/libvorbis-1.3.7.tar.xz
LIBVORBIS_SRC=$PROJECT_LIBDIR/libvorbis-1.3.7

LIBDAV1D_TAR=$PROJECT_LIBDIR/dav1d-1.1.0.tar.xz
LIBDAV1D_SRC=$PROJECT_LIBDIR/dav1d-1.1.0


ASCII_VIDEO_BUILD=$PROJECT_ROOT/build

BUILDING_DEPS="git \
autoconf \
automake \
build-essential \
make \
cmake \
git \
libtool \
pkg-config"

FFMPEG_DEPS="meson \
ninja-build \
yasm"

NCURSES_APT="libncurses6 \
libncurses-dev"

APT_DEPS_TO_INSTALL=""

check_debian() {
    [[ -f "/etc/debian_version" ]]
    return $?
}


cleanup() {
    rm -rf $FFMPEG_SRC $NCURSES_SRC $LIBVORBIS_SRC $LIBDAV1D_SRC
}

trap cleanup EXIT

check_debian
IS_DEBIAN=$?

nproc --all
N_PROC=$?
MAKE_PROC=1
if [[ $N_PROC -gt 1 ]]
then
    MAKE_PROC=$(($N_PROC / 2))
fi

if [[ ! $IS_DEBIAN -eq 0 ]]; then
   echo "Cannot build $PROJECT_NAME. Project can only currently build \ 
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
            *) echo "invalid input"
        esac 
    done  
}

check_ncurses6_installed_globally() {
    if ldconfig -p | grep "libncurses.so.6\|libncursesw.so.6\|libncurses.so.5\|libncursesw.so.5" >/dev/null 2>&1;
    then
        return 0
    else
        return 1
    fi
}


install_local_ncurses() {
    echo "Creating local installation of ncurses6 for ascii_video"
    rm -rf $NCURSES_SRC $NCURSES_BUILD
    tar -xvf $NCURSES_TAR -C $PROJECT_LIBDIR
    cd $NCURSES_SRC
    mkdir -p $NCURSES_BUILD
    mkdir -p $NCURSES_DATADIR

    ./configure \
    --prefix=$NCURSES_BUILD \
    --datadir=$NCURSES_DATADIR \
    --without-tests \
    --without-manpages \
    --without-progs


    make -j$MAKE_PROC
    make install
    cd $PROJECT_ROOT
    rm -rf $NCURSES_SRC
    return 0
}

# install_local_libvorbis() {

# }

install_local_ffmpeg() {
    mkdir -p $FFMPEG_BUILD
    rm -rf $FFMPEG_SRC
    tar -xvf $FFMPEG_TAR -C $PROJECT_LIBDIR
    cd $FFMPEG_SRC

    PKG_CONFIG_PATH="$FFMPEG_BUILD/lib/pkgconfig"

    ./configure \
    --prefix="$FFMPEG_BUILD" \
    --pkg-config-flags="--static" \
    --extra-cflags="-I$FFMPEG_BUILD/include" \
    --extra-ldflags="-L$FFMPEG_BUILD/lib" \
    --extra-libs="-lpthread -lm" \
    --ld="g++" \
    --disable-shared \
    --enable-static \
    --enable-libdav1d \
    --enable-libvorbis \
    --disable-network \
    --disable-encoders \
    --disable-muxers \
    --disable-programs \
    --disable-doc \
    --disable-filters \
    --disable-postproc \
    --disable-avfilter \
    --disable-avdevice \
    --disable-protocols \
    --enable-protocol=file,pipe \
    --disable-devices

    make -j$MAKE_PROC
    make install
    hash -r
    rm -rf $FFMPEG_SRC
    cd $PROJECT_ROOT
    return 0
}

# install_local_libdav1d() {

# }

dep_exists() {
    $1 --version >/dev/null 2>&1 
    return $?
}


echo "Building $PROJECT_NAME by $AUTHOR. Version $VERSION. See source code at $GITHUB_URL"

# Use the sudo command if the user is not root
SUDO=''
if [[ ! $EUID -eq 0 ]]
then
    SUDO='sudo'
fi

# Basic dependencies needed to build software on Debian-Type distributions 
# These will be needed for ncurses and building ascii_video itself

if ! dep_exists git;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL git"
fi

if ! dep_exists autoconf;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL autoconf"
fi

if ! dep_exists automake;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL automake"
fi

if ! dep_exists make;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL make"
fi

if ! dep_exists cmake;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL cmake"
fi

if ! dep_exists pkg-config;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL pkg-config"
fi

if ! dep_exists meson;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL meson"
fi


if ! dep_exists ninja;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL ninja"
fi

if ! dep_exists yasm;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL yasm"
fi

if ! check_ncurses6_installed_globally;
then
    APT_DEPS_TO_INSTALL="$APT_DEPS_TO_INSTALL libncurses6 libncurses-dev"
fi

if [[ ! "$APT_DEPS_TO_INSTALL" = "" ]]
then
    echo "Installing dependencies $APT_DEPS_TO_INSTALL from apt repositories"
    $SUDO apt-get update
    $SUDO apt-get install $APT_DEPS_TO_INSTALL
else 
    echo "Found all required building dependencies installed: $BUILDING_DEPS $FFMPEG_BUILD $NCURSES_APT"
fi

mkdir -p $PROJECT_LIBDIR_BUILD

if ! check_ncurses6_installed_globally;
then
    echo "Could not find ncurses installation: Installing ncurses locally"
    install_local_ncurses
fi

if [[ ! -d $FFMPEG_BUILD ]]
then
    install_local_ffmpeg
fi


rm -rf $ASCII_VIDEO_BUILD
mkdir $ASCII_VIDEO_BUILD
cd $ASCII_VIDEO_BUILD
cmake ../
make -j$MAKE_PROC
