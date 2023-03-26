
# ascii_video ChangeLog

## March 26, 2023

Added install.sh, which builds the ncurses and ffmpeg dependencies for ascii_video as static libraries from the tarballs
located in the lib folder. The build files are located in lib/build for each dependency, and will be located in
lib/build/ncurses-6.4 for ncurses and lib/build/ffmpeg-6.0 for ffmpeg.

- Currently only tested install.sh building on my personal machine in Ubuntu 22.04 and a Docker container running Ubuntu 22.04
- Definitely does not work outside of Debian Linux, as apt-get is used to install some dependencies for the ffmpeg build

Added docstring for definitions of VideoOutputMode transitions in renderer.cpp

Changed AudioStream object to be named AudioBuffer

Changed includes/audiostream.h to includes/audiobuffer.h

Changed src/audiostream.cpp to src/audiobuffer.cpp

Changed src/tests/test_audiostream.cpp to src/tests/test_audiobuffer.cpp

Changed MediaPlayer::audio_stream to MediaPlayer::audio_buffer

Changed AudioBuffer::m_stream to AudioBuffer::m_buffer

Readded old functionality of decoding media streams until target_time on MediaPlayer::jump_to_time after avformat_seek_file

Upon a desync between audio and video, added 3 possibilities for behaviors to resync

- If there are samples loaded into the AudioBuffer which are at the correct timestamp, jump the AudioBuffer to those samples
- If the audio timestamp is before the correct timestamp and the time until the correct time stamp is less than or equal to MAX_AUDIO_CATCHUP_DECODE_TIME_SECONDS, load audio packets into the audio buffer until the audio buffer catches up to the correct time stamp. If this fails, jump to the specified time using MediaPlayer::jump_to_time
- If these first two conditions are false (If the audio timestamp is after the correct timestamp or the audio timestamp is before the correct time stamp and farther than MAX_AUDIO_CATCHUP_DECODE_TIME_SECONDS seconds away), jump to the specified time using MediaPlayer::jump_to_time

Removed includes/aabb.h and includes/aabb.cpp, as they are not planned to be used anytime soon and served no purpose

Removed government name from title in README.md

Bumped down all release versions in README.md from 1.x to 0.x, as the API and program capabilities are not consistent as of yet

Scaled all gifs in README.md down, as they were way too big

Removed Description section from README.md that detailed on my motivation to create ascii_video, as it's pretty outdated and I just like this program now

Added links to all dependencies' home pages to README.md

Added Similar Programs section with links to README.md

Moved Example Output in README.md to the top of the file

Added the main typing-type gif to the bottom and top of README.md

Simplified project description in README.md

Changed install.sh name to build.sh

Added install.sh script. Creates ~/.local/bin if it does not exist. Adds ~/.local/bin to path if it did not exist, and
copies the ascii_video binary to ~/.local/bin

Updated Ubuntu Install in README.md to adjust to new build scripts

Fixed README.md dependencies note to talk about how system-installable versions of ncurses and ffmpeg are linked to if they are not built by ascii_video

Optimized gifs README.md using gifsicle and moved them to assets/readme
