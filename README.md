# tmedia

Linux C++ 17 Filesystem Terminal Media Player

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
  - 'L' - Switch looping type of playback (no loop, loop, and loop one)
  - 'M' - Mute/Unmute Audio
- Video, Audio, and Image Controls
  - 'C' - Display Color (on supported terminals)
  - 'G' - Display Grayscale (on supported terminals)
  - 'B' - Display no Characters (on supported terminals) (must be in color or grayscale mode)
  - 'N' - Skip to Next Media File
  - 'P' - Rewind to Previous Media File
  - 'R' - Fully Refresh the Screen

## CLI Arguments

```txt
Positional arguments:
  paths                  The the paths to files or directories to be
                         played. Multiple files will be played one
                         after the other Directories will be expanded
                         so any media files inside them will be played.


Optional arguments:
  Help and Versioning:
  -h, --help             shows help message and exits
  --help-cli             shows help message for cli options and exits
  --help-controls        shows help message for tmedia controls and exits
  -V, --version          prints version information and exits
  --ffmpeg-version       Print the version of linked FFmpeg libraries
  --curses-version       Print the version of linked Curses libraries
  --fmt-version          Print the version of the fmt library
  --miniaudio-version    Print the version of the miniaudio library
  --lib-versions         Print the versions of third party libraries

Video Output:
  -c, --color            Play the video with color
  -g, --gray             Play the video in grayscale
  -b, --background       Do not show characters, only the background
  -f, --fullscreen       Begin the player in fullscreen mode
  --show-ctrl-info       Begin the player showing control info (Default Yes)
  --hide-ctrl-info       Begin the player hiding control info (Default Yes)
  --refresh-rate         Set the refresh rate of tmedia
  --chars [STRING]       The displayed characters from darkest to lightest

Audio Output:
  -v, --volume FLOAT[%]  Set initial volume ([0.0, 1.0] or [0.0%, 100.0%])
  -m, --mute, --muted        Mute the audio playback
Playlist Controls:
  --no-loop, --no-repeat     Do not repeat the playlist upon end (Default)
  -l, --loop, --repeat       Repeat the playlist upon playlist end
  --loop-one, --repeat-one   Start the playlist looping the first media
  -s, --shuffle              Shuffle the given playlist

Stream Control Commands:
  --enable-audio-stream  (Default) Enable playback of audio streams
  --enable-video-stream  (Default) Enable playback of video streams
  --disable-audio-stream  Disable playback of audio streams
  --disable-video-stream  Disable playback of video streams
  :enable-audio-stream    Enable playback of audio streams for last path
  :enable-video-stream    Enable playback of video streams for last path
  :disable-audio-stream   Disable playback of audio streams for last path
  :disable-video-stream   Disable playback of video streams for last path

File Searching:
  NOTE: all local (:) options override global options

  -r, --recursive        Recurse child directories to find media files
  --ignore-images        Ignore image files while searching directories
  --ignore-video         Ignore video files while searching directories
  --ignore-audio         Ignore audio files while searching directories
  --probe, --no-probe    (Don't) probe all files
  --repeat-paths [UINT]    Repeat all cli path arguments n times
  :r, :recursive         Recurse child directories of the last listed path
  :repeat-path [UINT]    Repeat the last given cli path argument n times
  :ignore-images         Ignore image files when searching last listed path
  :ignore-video          Ignore video files when searching last listed path
  :ignore-audio          Ignore audio files when searching last listed path
  :probe, :no-probe      (Don't) probe last file/directory
```

All of these controls can also be seen when calling tmedia with no args or
with --help

## Installing

See [BUILD.md](./doc/BUILD.md) for information. Currently, Ubuntu 20.04
is the main tested system for tmedia.

## Bug and Feature Reporting

If there are any problems with installing or using tmedia, or any requested feature,
please leave an issue at the [github repo](https://www.github.com/cobyj33/tmedia)
and I'll be sure to help.

## Acknowledgments

### Bundled 3rd-party libraries

See [lib/README.md](./lib/README.md)

### Inspiration and Resources

* [ncurses 3D renderer](https://github.com/youngbrycecode/RenderEngine)
  * [video-series](https://www.youtube.com/playlist?list=PLg4mWef4l7Qzxs_Fa2DrgZeJKAbG3b7ue)
  * created by [youngbrycecode](https://github.com/youngbrycecode)
* [Coding Video into Text by The Coding Train on Youtube](https://www.youtube.com/watch?v=55iwMYv8tGI)
  * [The Coding Train](https://www.youtube.com/c/TheCodingTrain)
* [How to Write a Video Player in Less Than 1000 Lines](http://dranger.com/ffmpeg/)
