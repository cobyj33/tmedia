This document is also written so that someone with very little knowledge of
cmake could still build the library fairly easily. If there is any confusion, 
questions, or criticisms, please don't hesitate to leave an issue on Github at
https://github.com/cobyj33/tmedia/issues with any questions


This document will

## 


## Building Dependencies


## Finding an external FFmpeg

Simply add PKG_CONFIG_PATH="prefix-of-ffmpeg-pkgconfig" before your cmake command

```bash
# Example, starting from the source directory
mkdir ./build
cd ./build
PKG_CONFIG_PATH="$HOME/compiled/ffmpeg/master/build/lib/pkgconfig" cmake ..
```
