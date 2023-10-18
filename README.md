
# ASCII Video

![gif](/assets/readme/ascii-320.gif)

Terminal video player, Written in C++ 17

## Example Output

![example created in tmux](assets/readme/example-320.gif)
![example colored output](assets/readme/colored_music_record-160.gif)
![example vscode](assets/readme/vscode.png)
![example volcano](assets/readme/volcano.gif)
![example keybiard](assets/readme/keyboard_man.gif)

## Dependencies

* [curses](https://invisible-island.net/ncurses/)
* [FFmpeg](https://ffmpeg.org/)
* [cmake](https://cmake.org/)

## Installing

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config libavdevice-dev libncurses-dev
```

run these 4 commands in the terminal, starting from
the root of the repository:

```bash
mkdir build
cd build
cmake ../
make -j2
```

The file "ascii_video" should appear once the build finishes, and can be symlinked
to an area on your PATH if you want to use it anywhere

On other distros, ascii_video's installation is currently untested, but I don't see why
it wouldn't be possible

On non Linux/Unix, good luck for now, I'll get to y'all later

## Acknowledgments

Inspiration, code snippets, etc.

* [ncurses 3D renderer](https://github.com/youngbrycecode/RenderEngine)
  * [video-series](https://www.youtube.com/playlist?list=PLg4mWef4l7Qzxs_Fa2DrgZeJKAbG3b7ue)
  * created by [youngbrycecode](https://github.com/youngbrycecode)
* [Coding Video into Text by The Coding Train on Youtube](https://www.youtube.com/watch?v=55iwMYv8tGI)
  * [The Coding Train](https://www.youtube.com/c/TheCodingTrain)
* [How to Write a Video Player in Less Than 1000 Lines](http://dranger.com/ffmpeg/)

## Similar Programs

* [hiptext](https://github.com/jart/hiptext)
* [video-to-ascii](https://github.com/joelibaceta/video-to-ascii)

![gif](/assets/readme/ascii-320.gif)
