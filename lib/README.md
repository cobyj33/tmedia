
# Libraries and their Uses

## argparse | [Github](https://github.com/p-ranav/argparse)

### A Modern C++ 17+ Command-Line Argument Parser

Parsing Command-Line arguments is **a lot harder** than it initially seemed, so argparse allows me to easily follow standard CLI conventions
without the hassle of testing if my own implementation of reading CLI arguments meets the standard expectation.

## miniaudio | [Website](https://miniaud.io/) [Github](https://github.com/mackron/miniaudio)

### Audio playback and capture library written in C, in a single source file

Used as a cross-platform API to playback audio data from the video at a low-level. Miniaudio allows audio to be decoded as a stream and
sent back into the speakers as a float array, which allows tmedia to have high control over how audio is played, synced, and fetched, something that a simpler API wouldn't allow.

## Catch2 | [Github](https://github.com/catchorg/Catch2)

A testing framework for C++ 11+, which allows me to unit test sections of my code and focus on Test-Driven Development. With something complicated like a video player, Catch2 allows me to make sure none of my code breaks unexpectedly down the development line, which would lead to hours of debugging and lost time. Instead, Catch2 could catch any errors that are mistakenly made and isolate the issue, as well as promoting a more modular, decoupled build of tmedia.

## Natural Sort | [Github](https://github.com/scopeInfinity/NaturalSort)

Used to properly order playback of audio files in a directory, especially in playlist/album/mixtape
type directories where each song starts with a number indicating it's location in
the tracklist

## readerwriterqueue | [Github](https://github.com/cameron314/readerwriterqueue)

A single producer single consumer wait-free and lock-free fixed size queue written in C++11.
Used for lock-free realtime audio output.

## random | [Github](https://github.com/effolkronium/random)

A nice C++11 random number library, used only for shuffling playlists as of now.