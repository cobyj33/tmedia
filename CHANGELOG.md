
# ascii_video ChangeLog

## March 26, 2023

### Public Changes

Added install.sh, which builds the ncurses and ffmpeg dependencies for ascii_video as static libraries from the tarballs
located in the lib folder. The build files are located in lib/build for each dependency, and will be located in
lib/build/ncurses-6.4 for ncurses and lib/build/ffmpeg-6.0 for ffmpeg.

- Currently only tested install.sh building on my personal machine in Ubuntu 22.04 and a Docker container running Ubuntu 22.04
- Definitely does not work outside of Debian Linux, as apt-get is used to install some dependencies for the ffmpeg build

Added docstring for definitions of VideoOutputMode transitions in renderer.cpp
