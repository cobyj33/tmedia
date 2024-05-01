# CLI Arguments

This document contains more robust descriptions of how each CLI argument and
option works compared to the help text described by the top-level
[README.md](../README.md) or by entering ```tmedia```,
```tmedia -h``` or ```tmedia --help``` into the command line.

## CLI Argument Format

You can think of paths listed on the command line as having 'attributes' that
are affected by the options listed. 

Options that begin with the ```:``` character only apply to the previously
listed path on the command line. These options are referred to as 'local' 
options, as they are local to the previously listed path. Options that start
with ```-``` or ```--``` are 'global' options and have effect over all paths on
the command line. If there is no previously mentioned path on the command line,
'local' options have no effect. If a local option and its corresponding global
option are both specified on the command line, such as ```:probe``` and
```--no-probe```, the local option will always take precedence
over the global option. 

Note that when reading files from
[glob](https://en.wikipedia.org/wiki/Glob_(programming)) expressions, such as
```tmedia ~/Music/*.mp3```, then local options will only work on the final
matched path expanded by that globbing expression. This is because the shell
itself expands the glob expression to all matching paths before tmedia is even
called, so tmedia cannot tell whether filenames passed into tmedia are a result
of a globbing expression or were listed explicitly on the command line.

If a path is listed multiple times on the command line, it will simply be read
multiple times by tmedia.

Paths, unless specified by the options ```:probe``` and/or ```--probe```, are
not opened to check if they are valid media files, but are instead assumed
to be a certain media type by the given file extension that they have. For 
example, ```image.jpeg``` would be assumed to be an image and ```dancing.mp4```
would be assumed to be a video.

> If a file is not what it claims to be, such as if a video file is not a video
> file, it'll still be read as whatever it is upon playback. If it's not even
> a media file, then it'll just be discarded from the playlist upon attempted
> playbacj.
>
> This shouldn't actually happen ever with like 99.9% of people unless it
> happens on purpose, but it is handled. 

## Positional arguments
- paths
  - Any CLI argument not given as an option or a parameter to an option is
    interpreted as a path to a media file. 1 or more paths can be entered
    to begin playback. Multiple files will be played one after the other in
    the order provided on the command line. Directories will be expanded
    so any media files inside them will be played. Directories are traversed
    shallowly by default, a behavior which can be changed by the
    ```-r```, ```:r```, ```--recursive```, and ```:recursive``` flags detailed
    under [File Searching](#file-searching).

## Optional arguments

### Help and Versioning 

For all of these commands, the CLI automatically exits whenever any of them 
are encountered. No paths have to be entered into the command line for any of 
the Help and Versioning commands to work.

- -h, --help
  - Displays the help message and exits. The help message contains information
    about the in-player controls and short explanations of each command line
    option. This option can also be emulated if no arguments at all are given
    to the tmedia executable.
- -v, --version
  - Prints version information in the form MAJOR.MINOR.PATH and exits.  
- --ffmpeg-version
  - Prints version information of the used ffmpeg libraries and exits.
- --curses-version       
  - Prints version information of the used curses libraries and exits.
- --fmt-version       
  - Prints version information of the used fmt library and exits.
- --fmt-version       
  - Prints version information of the used miniaudio library and exits.
- --lib-versions
  - Prints the version information of all third-party libraries and exits.

### Video Output

- -c, --color 
  - Display the given media with color output enabled on startup. Color output can
  be altered at any time interactively in the actual media player with
  the 'c' key.
- -g, --gray             Play the video in grayscale 
  - Display the given media with grayscale output enabled on startup.
  Grayscale output can be altered at any time interactively in the actual media
  player with the 'g' key.
- -b, --background       Do not show characters, only the background 
  - Display the given media with only background colors on startup. This option
  only has any effect if --color (-c) or --gray (-g) are also
  passed into the command line. Background output can be altered at any time
  interactively in the actual media player with the 'b' key.

- -f, --fullscreen
  - Begin the player in fullscreen mode. Fullscreen mode can
  be altered at any time interactively in the actual media player with
  the 'f' key.

- --refresh-rate [UINT]     
  - Set the refresh rate of the terminal input and screen in frames per second.
  Defaults to 24 FPS. This shouldn't really ever have to be altered, but is
  available. The given parameter must be greater than 0.

- --chars [STRING]
  - Sets the characters used to display frames in the media player. The string
  is interpreted as a linear ramp between the 'darkest' character on the left
  and the 'lightest' character on the right. For example, the in string 
  ```@#$0l|io_```, '@' would be interpreted as approaching black, '_' would
  be interpreted as approaching white, and 'l' would be interpreted as halfway
  between black and white. It's best to use characters which "fill" the font
  for blacker characters and characters which are mostly whitespace as whiter
  characters. Whitespace itself could also be included in the string for 
  --chars.
  - Note that listing characters inside a single-quoted string would be desired
  for the parameter so that special characters like ```$``` and ```|```
  don't get interpreted by the shell before reaching tmedia.

### Audio Output 
- --volume [FLOAT] || [INT%]
  - Set initial volume used upon startup when using the media player. Input can
    be entered as a floating point value in the inclusive range 0.0 to 1.0,
    or an integer in the inclusive range 0% to 100% where the '%' sign is
    required. Volume can be changed interactively during playback with the 
    up arrow and down arrow keys.

- -m, --mute, --muted
  - Mute the audio output upon startup. This can be changed interactively
    during playback with the 'm' key.

### Playlist Controls 

See [playlist.md](./playlist.md) for details about looping types, shuffling, 
and the loaded media playlist in general. (Things work how you'd expect)

- --no-repeat
  - The default looping state of the media player. Upon reaching the end of the
    playlist, do not repeat the playlist.
- --repeat, --loop
  - Upon reaching the end of the loaded playlist, repeat the playlist.
- --repeat-one
  - Upon reaching the end of the first media file in the playlist, repeat
    that media file.
- -s, --shuffle
  - Shuffle all inputted media files before first playback.


### File Searching
> NOTE: all local (:) options override global options

Note that some of these options can be emulated through other means
through a shell, but are provided as shorthands or conveniences. 
For example, ```recursive``` could easily be emulated by globs on bash shells
(i.e. ```tmedia ~/Music/* ~/Music/**/*``` instead of ```tmedia ~/Music -r```)
and ```repeat-path``` could easily be emulated by just listing a path on the
command line multiple times.

- -r, --recursive
  Recurse the directories of the listed paths, detecting media
  paths along the way. For files, this option has no effect.
  
- --ignore-images        Ignore image files while searching directories
- --ignore-video         Ignore video files while searching directories
- --ignore-audio         Ignore audio files while searching directories
  - Ignore image, video, or audio files when searching directories. Ignoring
    files is determined through the file's extension unless probing
    is enabled with ```--probe``` or ```:probe```. For listed files, those files
    are ignored if they match the ignoring criteria determined by one of these
    flags.

- --probe, --no-probe    (Don't) probe all files
  - Probe (Open and read) all given files on the command line to determine if
    that file is a media file and to determine what type of media file that
    file is.
  - Generally, this would only be used if tmedia cannot determine what type of
    media is being played from the file's extensions, which should only be true
    for obscure file extensions.

- --repeat-paths [UINT]    Repeat all cli path arguments n times
  - Repeatedly read all  cli paths n times. The given argument must
    be an integer greater than 0. As an example, if 
    ```--repeat-paths 0``` is passed, then all paths are read once just like
    the default behavior. If
    ```--repeat-paths 1``` is passed, then the path is read twice. This
    acts as a more readable option to listing all path multiple times
    explicitly.

- :r, :recursive         Recurse child directories of the last listed path 
  - Recurse the child directories of the last listed path multiple times,
  detecting media paths along the way. If the last listed path is a file,
  then this option has no effect.

- :repeat-path [UINT]    Repeat the last given cli path argument n times
  - Repeatedly read the last given cli path n times. The given argument must
    be an integer greater than 0. As an example, if 
    ```:repeat-path 0``` is passed, then the last listed path is read once just
    like the default behavior. If
    ```:repeat-path 1``` is passed, then the last listed path is read twice.
    This acts as a more readable option to listing a path multiple
    times explicitly.
  - ```:repeat-path``` and ```--repeat-paths``` do not stack. For example, if
    ```tmedia ~/Music/song.mp3 :repeat-path 2 ~/Videos/vid.mp4 --repeat-paths 3```
    was invoked, then ```~/Music/song.mp3``` would be read 2 times and
    ```~/Videos/vid.mp4``` would be read 3 times.

- :ignore-images         Ignore image files when searching last listed path
- :ignore-video          Ignore video files when searching last listed path
- :ignore-audio          Ignore audio files when searching last listed path
  - Ignore image, video, or audio files when searching the last listed path.
    Ignoring files is determined through the file's extension unless probing is
    enabled. If the last listed path is a directory, then all
    files read from that directory have this attribute enabled. If the last
    listed path is a file, then the file is ignored if it matches the ignored
    media type.

- :probe, :no-probe      (Don't) probe last file/directory
  - Probe (Open and read) the last listed path on the command line to determine
    if the last listed path is a media file and to determine what type of
    media file the last listed path is. If the last listed path is a directory,
    then this option applies to all files found from the directory at the
    last listed path.
  - Generally, this would only be used if tmedia cannot determine what type of
    media is being played from the file's extensions, which should only be true
    for obscure file extensions.




