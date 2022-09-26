@\%#*+=-:._@\%#*+=-:._@\%#*+=-:._@\%#*+=-:._@\%#*+=-:._

# ASCII Video by Jacoby Johnson

A terminal video player and image display with a pure ASCII character display

![gif](/assets/readme/ascii.gif)

## Description

* Reasons for creating  
  * Interest: I figured it would be interesting to understand how audio and video worked at a lower level, and an app to let that data be displayed in the simplest and most practical form I could think of, the terminal, seemed like a great idea
  * Learning: This program has taught me about Video Codecs, Audio Data Storage, PGM Data, Memory Allocation and Management, C Programming Patterns, Multithreading topics like Mutual Excluison and Locking, Pointers, and how different streams of data are formatted and synced with one another
* How does it work
  * Simply, ASCII Video uses ffmpeg to open a video or image file and decode its images into grayscale, then uses a string of characters "@\%#*+=-:._ " to map different ascii characters to the grayscale pixels, and prints them to the screen with the ncurses library

## Dependencies

* Curses
* ffmpeg (recommended to build from source code)
* miniaudio (included in project already, no need to download)
* cmake


## Installing

### Windows
  Sorry, might be out of luck
  The Windows Subsystem for Linux may be your best bet though, in order to create a Linux environment on a Windows Computer

### Linux

#### Docker
+ This git repo ships with a Dockerfile which can be built with  
  ``` docker build -t ascii_video ```  
  
  Then, after the image is built, the program can be run with  
  ``` docker run --rm -it --device=/dev/snd -v=<PATH TO VIDEO>:/video.mp4 ascii_video -v /video.mp4```  
  Where ```<PATH TO VIDEO>``` is an actual file path to a video on your local machine  
  * Note: /dev/snd is used on here as it is the path for the standard sound device files on linux devices  
  * Working on a shorter command soon, as this is somewhat a lot to handle  
  * May need to grant admin privileges to docker commands using ```sudo ``` before all commands, or ```sudo ```'s alternative on your distro

#### Native Install

* First, download all dependencies

  * Curses: Should already be installed on ubuntu machines, but tutorials can be found [here](https://www.cyberciti.biz/faq/linux-install-ncurses-library-headers-on-debian-ubuntu-centos-fedora/)
    * Recommended to simply use the ncurses that comes with distro
    * Ex: On Ubuntu run ```sudo apt-get install libncurses5-dev libncursesw5-dev```

  * ffmpeg: Recommended to download from source, as I've experienced that atleast the Ubuntu repo was outdated for my needs. Source code can be found [here](https://github.com/FFmpeg/FFmpeg) with compilation instructions [here](https://trac.ffmpeg.org/wiki/CompilationGuide)

  * cmake: any download form, distro package is recommended
    * Ex: On Ubuntu run ```sudo apt-get install cmake```

  * also, software like gcc, g++, and make are needed to build software
    * Ex: On Ubuntu, simply run ```sudo apt-get install build-essential``` to get packages necessary for building C and C++ software

* Clone this repository into a folder (Will refer to this folder as <PROJECT> from now on)
  ``` git clone https://github.com/cobyj33/ascii_video.git <PROJECT>```
  * You can also download straight from github if you'd rather do that
  * \*Replace Project with your actual project folder location
* open your <PROJECT> folder in the terminal
* execute this code block
```
mkdir build && 
cd build && 
cmake ../ &&
make -j2
```
* Finished, the program should now work.
  * if you would like to add the directory directly to your path, run
``` cp -R ./ascii_video ~/.local/bin ```
  * Now, you should be able to use the ```ascii_video``` command without a ./ or any other qualifying path
    * If this does not work, ~/.local/bin may not be a part of your path  
      Try adding ~/.local/bin to path by editing ~/.bashrc and adding
      ```export PATH="~/.local/bin:$PATH"```



## Executing program

* use -h when executing in order to see commands
```
<executable> -v <path-to-file>: Play a video File
<executable> -v -c <path-to-file>: Play a video File with color (works if supported in the current terminal)
<executable> -i <path-to-file>: display image file
<executable> -info <path-to-file>: Get stream info about a multimedia file
code blocks for commands
```

## Help

* Open an issue or run
```
<executable> -h
```
* Will be glad to help anything that is not working

## Authors

Jacoby Johnson: [@cobyj33](https://www.github.com/cobyj33)

## Version History
* 1.3
  * Migration from C++ to fully C
  * Cleaner audio output, all media streams now share one single time state
  * Much more concise code, no more std::this::is::annoying::to::read
* 1.2
  * Added support for colored output with the -c flag before entering the file path
    * At most, uses 256 unique colors
* 1.1
    * Current Release, Mostly personal refactoring of the code to be more readable by separating the VideoState class into multiple Media classes
    * Removed Delay on changing video time
    * Fixed (although not in the best way) ending of the video files
    * Goals
      * Add Subtitle Support, where they will be printed in their own section at the bottom of the screen
      * If you look at the code, it is very C-ish, so I plan to turn it into a fully C program soon
      * Improve Debugging by adding different screen views to see the current program data, similar to the the "Stats for Nerds" section on Youtube
      * Make audio more smooth, as it sometimes crackles and is more apparent with higher volume
      * A way to jump to a specific time in the video, as well as a playback bar to see the current time of the video relative to the video's duration
* 1.0
    * Initial Release, Experimenting with speeding up and slowing down video, as well as changing video time dynamically. Video and image displays great and audio and video are synced, although audio may be a little scratchy from time to time
    * Plans for change: Moving all code out of a single monolithic, concrete class. Currently, the structure of the code makes it difficult to create new features as everything is so tightly coupled

## License

This project isn't licensed, do anything at all with it :) it was just for fun, even though it was a pain at times

## Acknowledgments

Inspiration, code snippets, etc.
* [ncurses 3D renderer](https://github.com/youngbrycecode/RenderEngine)
  * [video-series](https://www.youtube.com/playlist?list=PLg4mWef4l7Qzxs_Fa2DrgZeJKAbG3b7ue)
  * created by [youngbrycecode](https://github.com/youngbrycecode)
* [Coding Video into Text by The Coding Train on Youtube](https://www.youtube.com/watch?v=55iwMYv8tGI)
  * [The Coding Train](https://www.youtube.com/c/TheCodingTrain)
* [How to Write a Video Player in Less Than 1000 Lines](http://dranger.com/ffmpeg/)
  * \*Somewhat outdated but still a great resource for understanding the workings of a video decoding program

## Example Output
![example created in tmux](assets/example.gif)
![example colored output](assets/colored_music_record.gif)

<hr>
@\%#*+=-:._@\%#*+=-:._@\%#*+=-:._@\%#*+=-:._@\%#*+=-:._