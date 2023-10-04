
# DEV NOTES

## Known Bugs

On AVPackets, sometimes the time_base is not set, so don't rely on finding the time base for a stream through a packet

April 8: 2023, Found a bug where if the audio thread is started before the video thread, the function MediaPlayer::get_desync_time will fail.

October 3rd, 2023: av_frame_get_buffer has an alignment of 1, although ffmpeg
recommends 0. 0 doesn't work on my machine for proper resizing, and [other parts
of ffmpeg](https://stackoverflow.com/a/44894932) also sometimes use an align
of 1, so I'ma just leave it for now.