
# DEV NOTES

## Known Bugs

On AVPackets, sometimes the time_base is not set, so don't rely on finding the time base for a stream through a packet

April 8: 2023, Found a bug where if the audio thread is started before the video thread, the function MediaPlayer::get_desync_time will fail.
