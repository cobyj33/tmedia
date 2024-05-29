This document is also written so that someone with very little knowledge of
cmake could still build the library fairly easily. If there is any confusion, 
questions, or criticisms, please don't hesitate to leave an issue on Github at
https://github.com/cobyj33/tmedia/issues with any questions

### Dependencies (Handled by the user)

* [curses**](https://invisible-island.net/ncurses/) - Terminal Output
* [FFmpeg**](https://ffmpeg.org/) - Multimedia decoding, scaling, and resampling
* [cmake](https://cmake.org/) - Build System
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) - Finding ffmpeg libs

### Ubuntu/Debian Linux

Instructions will start all the way from a fresh build of Ubuntu. Ubuntu 22.04
or later is definitely recommended.

First, in a terminal, run these commands to install all system dependencies
needed to build tmedia.

```bash
sudo apt-get update
```

```bash
sudo apt-get install git build-essential cmake pkg-config libavdevice-dev libncurses-dev
```

Next, clone this repository to your desired directory with git if you
have not and enter the tmedia project's root path.

```bash
git clone https://github.com/cobyj33/tmedia.git
```

```bash
cd ./tmedia
```

Then, make sure to pull all submodule code, such as the fmt library, into
the source tree as well. (If you get an error about a source directory fmt
not being found, you may have missed this step (I miss this step alot too)).

```bash
git submodule update --init
```

Finally, run these 4 commands in order from the tmedia project's root
directory in order to build the tmedia binary. 

```
mkdir build
cd build
cmake ../
cmake --build .
```

The file "tmedia" should appear once the build finishes, and can be
symlinked to an area on your
[PATH](https://www.digitalocean.com/community/tutorials/how-to-view-and-update-the-linux-path-environment-variable)
if you want to use it anywhere.

If you know CMake, more documentation about building can be found in the
CMakeLists.txt file at the top of the branch.

If these steps do not work for any reason, it is a **bug** and I'll handle
any issue submitted to Github as quickly as possible.

## Windows

Currently for windows, WSL 2 running Debian/Ubuntu must be used to run tmedia.

After running WSL 2 with Debian/Ubuntu Linux, you can follow the installation
directions under [Ubuntu/Debian Linux](#ubuntudebian-linux)

## Other

> Currently, anything outside of Ubuntu/Debian is not tested. That being said,
> if you have the libav libraries from FFmpeg and the curses libraries available, 
> building tmedia should still work with all dependencies, but would require
> more knowledge about compiling and linking C/C++ programs.
> If you need help with this, I'll be happy to help in an issue.

> **ADVANCED ASIDE:**
> ** Technically, building curses and FFmpeg from source on the target machine
> could work with  ```-DFIND_FFMPEG=OFF``` and ```-DFIND_CURSES=OFF``` passed
> to the cmake command while configuring the build directory. This may be a
> good place to start if trying to compile for a non Ubuntu/Debian Linux system
> (that's still Linux.) The inner workings of how FFmpeg and ncurses are
> fetched, built, and can be configured are available in the
> lib/cmake/ffmpeg.cmake and lib/cmake/ncurses.cmake files respectively.
> If your system libraries may be too old, then it may be worth it to
> try out these options to get a build running instead of trying to compile
> the newest ffmpeg and ncurses yourself

### Configuration Snippets

## Finding an external FFmpeg

Simply add PKG_CONFIG_PATH="prefix-of-ffmpeg-pkgconfig" before your cmake
command.

```bash
# Example, starting from the source directory, and if ffmpeg was compiled
in "$HOME/compiled/ffmpeg/master/build/lib/pkgconfig"

mkdir ./build
cd ./build
PKG_CONFIG_PATH="$HOME/compiled/ffmpeg/master/build/lib/pkgconfig" cmake ..
```

## Finding an external curses (ncurses)

```bash
# Example, starting from the source directory, and if ncurses was compiled
in "$HOME/compiled/ffmpeg/master/build/l"

mkdir ./build
cd ./build
cmake .. -DCMAKE_PREFIX_PATH=$HOME/compiled/ncurses/master/build
```

## Notes:

### Compiler Optimizations 

While building, it is common to want optimizations enabled for your compiler.
To enable optimizations, add the corresponding compiler flags for optimization
to -DCMAKE_CXX_FLAGS when configuring for cmake.

```bash
# For GCC or clang
cmake .. -DCMAKE_CXX_FLAGS=-O3
```