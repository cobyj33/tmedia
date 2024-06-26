# tmedia

C++ 17 Terminal Media Player

## Example Output

![example created in tmux](assets/readme/example-320.gif)
![example colored output](assets/readme/colored_music_record-160.gif)
![example vscode](assets/readme/vscode.png)
![example volcano](assets/readme/volcano.gif)
![example keybiard](assets/readme/keyboard_man.gif)
![example audio-playback](assets/readme/audio_playing_tmedia_480.png)

## Table of Contents

- [Supports](#supports)
- [Media Controls](#media-controls)
- [Installing](#installing)
  - [Dependencies](#dependencies)
  - [Ubuntu/Debian Linux](#ubuntudebian-linux)
- [Bug and Feature Reporting](#bug-and-feature-reporting)
- [Acknowledgements](#acknowledgments)
  - [Bundled 3rd-party libraries](#bundled-3rd-party-libraries)
  - [Inspiration and Resources](#inspiration-and-resources)

## Supports

- Playing Audio and Video Files
- Displaying Image files
- Displaying Colored and Grayscale output
- Audio and Video Controls (Seeking, Changing Volume, Muting, Pausing, Looping)
- Playing multiple files
- Reading directories for multiple files to play

## Media Controls

- Video and Audio Controls
  - Space - Play and Pause
  - Up Arrow - Increase Volume 5%
  - Down Arrow - Decrease Volume 5%
  - Left Arrow - Skip Backward 5 Seconds
  - Right Arrow - Skip Forward 5 Seconds
  - Escape or Backspace or 'q' - Quit Program
  - '0' - Restart Playback
  - '1' through '9' - Skip To n/10 of the Media's Duration
  - 'L' - Switch looping type of playback (between no loop, repeat, and repeat one)
  - 'M' - Mute/Unmute Audio
- Video, Audio, and Image Controls
  - 'C' - Display Color (on supported terminals)
  - 'G' - Display Grayscale (on supported terminals)
  - 'B' - Display no Characters (on supported terminals) (must be in color or grayscale mode)
  - 'N' - Skip to Next Media File
  - 'P' - Rewind to Previous Media File
  - 'R' - Fully Refresh the Screen

All of these controls can also be seen when calling tmedia with no args or
with --help

## Installing

See [BUILD.md](./BUILD.md) for information. Currently, Ubuntu 20.04 is the main
tested 

## Bug and Feature Reporting

If there are any problems with installing or using tmedia, or any requested feature,
please leave an issue at the [github repo](https://www.github.com/cobyj33/tmedia)
and I'll be sure to help. 

## Acknowledgments

### Bundled 3rd-party libraries

* [miniaudio](https://miniaud.io/) - Cross-platform audio playback ([github](https://github.com/mackron/miniaudio))
* [Natural Sort](https://github.com/scopeInfinity/NaturalSort) - Natural Sorting and Comparison: For sorting directory files
* [readerwriterqueue](https://github.com/cameron314/readerwriterqueue) - Lock free SPSC Circular Buffer for consistent realtime audio
* [random](https://github.com/effolkronium/random) - "Random for modern C++ with convenient API" (README.md)

### Inspiration and Resources

* [ncurses 3D renderer](https://github.com/youngbrycecode/RenderEngine)
  * [video-series](https://www.youtube.com/playlist?list=PLg4mWef4l7Qzxs_Fa2DrgZeJKAbG3b7ue)
  * created by [youngbrycecode](https://github.com/youngbrycecode)
* [Coding Video into Text by The Coding Train on Youtube](https://www.youtube.com/watch?v=55iwMYv8tGI)
  * [The Coding Train](https://www.youtube.com/c/TheCodingTrain)
* [How to Write a Video Player in Less Than 1000 Lines](http://dranger.com/ffmpeg/)
