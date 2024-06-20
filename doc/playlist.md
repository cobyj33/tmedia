# Playlist

## Static Playlist

The media files in the playlist are set whenever tmedia begins playing and
cannot be modified from within the program without exiting and re-entering the
program.

## Looping States

There are three valid looping states in tmedia, 'NO-LOOP', 'LOOP', and
'LOOP-ONE'. They do what you'd expect by their names, but for the
sake of robustness:

- NO-LOOP
  - The default looping state of the media player.
  - If the loaded playlist's last media file is finished or skipped, then
    the media player is exited.
  - If a user tries to jump back a media file from the first media file in the
    loaded playlist, then the first media file is simply repeated

- LOOP
  - The LOOP in LOOP refers to repeating the entire playlist. Whenever all
    media files in the loaded playlist are finished, the playlist loops
    back to the beginning and restarts. Similarly, if the user jumps back a
    media file from the first media file in the loaded playlist, then the
    last media file is reached.

- LOOP-ONE
  - If LOOP-ONE is enabled, then whenever the currently playing media file
    is finished, the currently playing media file will be repeated.
  - If the currently playing media is skipped (with the 'n' key), then the
    playlist is kicked out of the LOOP-ONE looping state and put into the
    LOOP looping state.

## Shuffling

Shuffling is constant when shuffling in LOOP mode. If close to the end of the
shuffled playlist is reached (currently, 2 media files away from the end),
then the playlist is automatically reshuffled.

Shuffling is done on the entire loaded playlist once to guarantee that every
media file in the playlist will play at least once before the playlist is
reshuffled.

Shuffling can be toggled with the 's' key when using the media player. Shuffling
while using the media player does not modify the currently playing media file
in the playlist.

Starting the media player in shuffled mode can be specified with the '-s' or
the '--shuffle' command line options. When shuffling with the command line, any
media file can play first.
