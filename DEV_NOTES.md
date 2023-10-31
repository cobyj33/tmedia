
# DEV NOTES

## Testing files

While I don't include images, videos, and audio directly into the tmedia
repo, here are some good sources that I use to make sure many different file
formats and sizes work well.

Images:

[The USC-SIPI Image Database](https://sipi.usc.edu/database/)
[Png Suite](http://www.schaik.com/pngsuite/)
[google imagetestsuite](https://code.google.com/archive/p/imagetestsuite/)
[pygif testsuite](https://github.com/robert-ancell/pygif/tree/main/test-suite)
[metadata-extractor-images](https://github.com/drewnoakes/metadata-extractor-images)

Audio:

You can download various mixtapes, audio files, and whatnot from
[Web Archive](https://archive.org/), which give an easy way to listen to music
and test at the same time

Video:

[Sample Videos](https://sample-videos.com/)
[Pexels](https://www.pexels.com/search/videos/sample/)

## Known Bugs and Quirks

On AVPackets, sometimes the time_base is not set, so don't rely on finding the time base for a stream through a packet

April 8: 2023, Found a bug where if the audio thread is started before the video thread, the function MediaPlayer::get_desync_time will fail.

October 3rd, 2023: av_frame_get_buffer has an alignment of 1, although ffmpeg
recommends 0. 0 doesn't work on my machine for proper resizing, and [other parts
of ffmpeg](https://stackoverflow.com/a/44894932) also sometimes use an align
of 1, so I'ma just leave it for now.