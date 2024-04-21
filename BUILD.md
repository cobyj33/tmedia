This document is also written so that someone with very little knowledge of
cmake could still build the library fairly easily. If there is any confusion, 
questions, or criticisms, please don't hesitate to leave an issue on Github at
https://github.com/cobyj33/tmedia/issues with any questions


This document will

### Dependencies

* [curses](https://invisible-island.net/ncurses/) - Terminal Output
* [FFmpeg](https://ffmpeg.org/) - Multimedia decoding, scaling, and resampling
* [cmake](https://cmake.org/) - Building
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) - Finding ffmpeg libs

### Ubuntu/Debian Linux

First, clone this repository to your desired directory if you have not done so
already with git.

```bash
git clone https://github.com/cobyj33/tmedia.git
```

Next, run the debian-ubuntu-bootstrap.sh script in the top-level of the
project's source **from wherever you cloned tmedia**.

```bash
chmod +x ./debian-ubuntu-bootstrap.sh
./debian-ubuntu-bootstrap.sh
```

> This script will use the apt package repository to download all dependencies
> and then build tmedia 

The file "tmedia" should appear once the build finishes, and can be
symlinked to an area on your PATH if you want to use it anywhere

## Windows

Currently for windows, WSL 2 running Debian/Ubuntu must be used to run tmedia.

After running WSL 2 with Debian/Ubuntu Linux, you can follow the installation
directions under [Ubuntu/Debian Linux](#ubuntudebian-linux)

> Currently, anything outside of Ubuntu/Debian is not tested. That being said,
> if you have the libav libraries from FFmpeg and the curses libraries available, 
> building tmedia should still work with all dependencies, but would require
> more knowledge about compiling and linking C/C++ programs.
>  If you need help with this, I'll be happy to help in an issue.

### Configuration Snippets

## Finding an external FFmpeg

Simply add PKG_CONFIG_PATH="prefix-of-ffmpeg-pkgconfig" before your cmake command

```bash
# Example, starting from the source directory
mkdir ./build
cd ./build
PKG_CONFIG_PATH="$HOME/compiled/ffmpeg/master/build/lib/pkgconfig" cmake ..
```
