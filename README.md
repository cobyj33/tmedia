
# ASCII Video

![gif](/assets/readme/ascii-320.gif)

Just another terminal video player

Written in C++ 17

> **ascii_video is still very much in testing stages, and is not guaranteed to**
> **run perfectly on non-Ubuntu 22.04 systems. Additionally, no pre-compiled**
> **binaries are currently provided, but a simple build command is**
> **compiled to be ran**

## Example Output

![example created in tmux](assets/readme/example-320.gif)
![example colored output](assets/readme/colored_music_record-160.gif)
![example vscode](assets/readme/vscode.png)

## Dependencies

* [argparse](https://github.com/p-ranav/argparse)
* [ncurses](https://invisible-island.net/ncurses/)
* [FFmpeg](https://ffmpeg.org/)
* [miniaudio](https://miniaud.io/)
* [cmake](https://cmake.org/)

Building:  
g++, gcc, libtool, make, cmake, pkg-config, yasm (on x86 or x86_64 cpu
architectures), git

> NOTE: **The source code for all non-building dependencies listed above**
> **are already included and will be compiled along with ascii_video**

## Installing

Currently, Linux Ubuntu and Debian builds have been tested and should work.

Non-Debian distributions of Linux are unsupported.

Windows and Mac O/S are unsupported.

Compilation has been tested with gcc version 11.3.0 and clang 14.0.0, and needs
a C++ 17 compiler to work

Currently, I'd recommend using a similar program like
[video-to-ascii](https://github.com/joelibaceta/video-to-ascii) on Mac or
Non-Debian Linux.

### Ubuntu Install

For Ubuntu, these building dependencies must be installed

```bash
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config git libtool
```

Additionally, if on a x86, x86_64, or x64 architecture, ```yasm and nasm```
must be installed through ```sudo apt-get install nasm yasm```.
The architecture could be checked with the command ```uname -m```.

run these 4 commands in the terminal, starting from
the root of the repository:

```bash
mkdir build
cd build
cmake ../
make -j2
```

The file "ascii_video" should appear once the build finishes, and can be copied
to an area on your path such as "usr/local/bin" or "$HOME/bin" to be used from
anywhere

On other distros, ascii_video's installation is currently untested.

> Additionally, ascii_video can work with a system-installed ncurses and a
> system-installed ffmpeg, but will not work with an ffmpeg libraries under
> release version less than 3.4. However, it is recommended to compile with
> the statically linked new ffmpeg versions for now.

Additional CMake options and configurations can be seen in the CMakeLists.txt
file for more advanced users accustomed to using CMake and building software.

### Uninstall

To uninstall, simply delete wherever the binary file has been placed.

## Executing program

> use -h, --help, or simply call the binary
> with no arguments in order to see the help message

Essentially, pass in a video file and that video will begin playback.

I would be glad to help anything that is not working in an open issue

## Other Notes

While it is by no means required, ascii_video will run faster in GPU accelerated
terminals like [alacritty](https://github.com/alacritty/alacritty) and
[kitty](https://github.com/kovidgoyal/kitty).

## Version History

Currently working toward version 0.5

* 0.4
  * Migration from C back to C++ for compatibility reasons
  * Addition of Catch2 for unit-testing and argparse for cli argument parsing
  * Removal of some features during rewrite such as visualizing audio channels
    and visualizing debug data. This was done to focus on more reliable playback
  * Improved syncing, seeking, and robustness of code
  * Much better documented and tested code
  * Added grayscale output support with the -g flag
  * Added build script to localize dependencies and more easily build the
    program from source
  * Added options to build tests or build as Debug or Release in CMakeList.txt
  * std::this::is::annoying::to::read is back as referred to in 0.3, but behind
    some more concise functions instead
  * Can skip around in video file, as well as start playing video file at a
    certain timestamp with the -t 0:00 flag
  * Much better, standardized, and flexible cli parsing with
    [argparse](https://github.com/p-ranav/argparse)
* 0.3
  * Migration from C++ to fully C
  * Cleaner audio output, all media streams now share one single time state
  * Much more concise code, no more std::this::is::annoying::to::read
* 0.2
  * Added support for colored output with the -c flag
    before entering the file path
  * At most, uses 256 unique colors
* 0.1
  * Mostly personal refactoring of the code to be more readable by
    separating the VideoState class into multiple Media classes
  * Removed Delay on changing video time
  * Fixed (although not in the best way) ending of the video files
  * Goals
    * Add Subtitle Support, where they will be printed in their own
      section at the bottom of the screen
    * If you look at the code, it is very C-ish, so I plan to turn
      it into a fully C program soon
    * Improve Debugging by adding different screen views to see the
      current program data, similar to the the "Stats for Nerds"
      section on Youtube
    * Make audio more smooth, as it sometimes crackles and is
      more apparent with higher volume
    * A way to jump to a specific time in the video, as well as a playback
      bar to see the current time of the video relative to the video's duration
* 0.0
  * Initial Release, Experimenting with speeding up and slowing down video,
    as well as changing video time dynamically. Video and image displays great
    and audio and video are synced, although audio may be a little scratchy
    from time to time
  * Plans for change: Moving all code out of a single monolithic,
    concrete class. Currently, the structure of the code makes it difficult
    to create new features as everything is so tightly coupled

## Acknowledgments

Inspiration, code snippets, etc.

* [ncurses 3D renderer](https://github.com/youngbrycecode/RenderEngine)
  * [video-series](https://www.youtube.com/playlist?list=PLg4mWef4l7Qzxs_Fa2DrgZeJKAbG3b7ue)
  * created by [youngbrycecode](https://github.com/youngbrycecode)
* [Coding Video into Text by The Coding Train on Youtube](https://www.youtube.com/watch?v=55iwMYv8tGI)
  * [The Coding Train](https://www.youtube.com/c/TheCodingTrain)
* [How to Write a Video Player in Less Than 1000 Lines](http://dranger.com/ffmpeg/)
  * Outdated toward current ffmpeg standards but still a great resource
    for understanding the workings of a video decoding program

## Similar Programs

* [hiptext](https://github.com/jart/hiptext)
* [video-to-ascii](https://github.com/joelibaceta/video-to-ascii)

![gif](/assets/readme/ascii-320.gif)
