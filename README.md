
# ASCII Video

![gif](/assets/readme/ascii-320.gif)

Just another terminal video player

## Example Output

![example created in tmux](assets/example-320.gif)
![example colored output](assets/colored_music_record-160.gif)

## Dependencies

* [argparse](https://github.com/p-ranav/argparse)
* [ncurses](https://invisible-island.net/ncurses/)
* [FFmpeg](https://ffmpeg.org/)
* [miniaudio](https://miniaud.io/)
* [cmake](https://cmake.org/)
* Build-essential tools, such as g++, gcc, make, libtool, etc...

> NOTE: The source code for ffmpeg, ncurses, miniaudio, and argparse are already included in this repository and will be compiled along with ascii_video
> Additionally, ascii_video can build frses and ffmpeg, but will not work with an ffmpeg version less than 6.0, which
> is why the source codes of ncurses and ffmpeg are included as tarballs to ensure a proper build

## Installing

### Currently Unsupported or Untested

Windows, Mac O/S, and really anything except for Ubuntu 22.04 is currently untested. (we're still in the early stages).
Currently, I'd recommend using a similar program like MacOS [video-to-ascii](https://github.com/joelibaceta/video-to-ascii) on Mac or Non-Debian Linux, although the Docker build should be able to work on any platform

#### Ubuntu Install

On Ubuntu, simply run this command from the cloned repository

```bash
chmod +x ./build.sh && ./build.sh
```

The binary should then be available inside of the build/ folder after the script finishes
  
To install globally for the current user, run these 3 commands:

```bash
[ ! -d ~/.local/bin ] && mkdir ~/.local/bin && echo "export $PATH=$HOME/.local/bin:$PATH" >> ".$(echo $SHELL | sed -nE "s|.*/(.*)\$|\1|p")rc"
cp -R ./build/ascii_video ~/.local/bin
hash -r
```

On other distros, ascii_video's installation is currently untested. However, this should theoretically work on all Debian Linux distributions.

#### Docker

This is only really recommended if you're familiar with Docker by the way, because I can see how it could be a pain for newcomers

First, [docker must be installed](https://docs.docker.com/engine/install/)

This git repo ships with a Dockerfile which can be built with

``` docker build -t ascii_video ```  
  
Then, after the image is built, the program can be run with  
```docker run --rm -it --device=/dev/snd -v=<PATH TO VIDEO>:/video.mp4 ascii_video -v /video.mp4```  
Where ```<PATH TO VIDEO>``` is an actual file path to a video on your local machine

> Note: /dev/snd is used on here as it is the path for the standard sound device files on linux devices  
> Note: May need to grant admin privileges to docker commands using ```sudo``` before all commands, or ```sudo```'s alternative on your distro

### Uninstall

Just delete wherever the binary is. It's fully self-contained.

## Executing program

> use -h, --help, or simply call the binary with no arguments in order to see the help message

Essentially, pass in a video file and it'll start playing. Keeping it simple.
To see more **colorful** options, call ascii_video with the -h flag

## Help

* Open an issue or run ```bash <path-to-executable> -h```
* Will be glad to help anything that is not working

## Version History

* 0.4
  * Migration from C back to C++ for compatibility reasons
  * Addition of Catch2 for unit-testing and argparse for cli argument parsing
  * Removal of some features during rewrite such as visualizing audio channels and visualizing debug data. This was done to focus on more reliable playback
  * Improved syncing, seeking, and robustness of code
  * Much better documented and tested code
  * Added grayscale output support with the -g flag
  * Added install script to localize dependencies and more easily install
  * Added options to build tests or build as Debug or Release in CMakeList.txt
  * std::this::is::annoying::to::read is back as referred to in 0.3, but behind some more concise functions instead
  * Can skip around in video file, as well as start playing video file at a certain timestamp with the -t 0:00 flag
  * Much better, standardized, and flexible cli parsing with [argparse](https://github.com/p-ranav/argparse)
* 0.3
  * Migration from C++ to fully C
  * Cleaner audio output, all media streams now share one single time state
  * Much more concise code, no more std::this::is::annoying::to::read
* 0.2
  * Added support for colored output with the -c flag before entering the file path
    * At most, uses 256 unique colors
* 0.1
  * Mostly personal refactoring of the code to be more readable by separating the VideoState class into multiple Media classes
  * Removed Delay on changing video time
  * Fixed (although not in the best way) ending of the video files
  * Goals
    * Add Subtitle Support, where they will be printed in their own section at the bottom of the screen
    * If you look at the code, it is very C-ish, so I plan to turn it into a fully C program soon
    * Improve Debugging by adding different screen views to see the current program data, similar to the the "Stats for Nerds" section on Youtube
    * Make audio more smooth, as it sometimes crackles and is more apparent with higher volume
    * A way to jump to a specific time in the video, as well as a playback bar to see the current time of the video relative to the video's duration
* 0.0
  * Initial Release, Experimenting with speeding up and slowing down video, as well as changing video time dynamically. Video and image displays great and audio and video are synced, although audio may be a little scratchy from time to time
  * Plans for change: Moving all code out of a single monolithic, concrete class. Currently, the structure of the code makes it difficult to create new features as everything is so tightly coupled

## Acknowledgments

Inspiration, code snippets, etc.

* [ncurses 3D renderer](https://github.com/youngbrycecode/RenderEngine)
  * [video-series](https://www.youtube.com/playlist?list=PLg4mWef4l7Qzxs_Fa2DrgZeJKAbG3b7ue)
  * created by [youngbrycecode](https://github.com/youngbrycecode)
* [Coding Video into Text by The Coding Train on Youtube](https://www.youtube.com/watch?v=55iwMYv8tGI)
  * [The Coding Train](https://www.youtube.com/c/TheCodingTrain)
* [How to Write a Video Player in Less Than 1000 Lines](http://dranger.com/ffmpeg/)
  * Somewhat outdated but still a great resource for understanding the workings of a video decoding program

## Similar Programs

* [hiptext](https://github.com/jart/hiptext)
* [video-to-ascii](https://github.com/joelibaceta/video-to-ascii)

![gif](/assets/readme/ascii-320.gif)
