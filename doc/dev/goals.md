# Goals

tmedia is largely in a state where I don't really want to work on new features
much anymore, as it already does everything that I realistically
want it to do. Perhaps a more complex and interactive TUI could be put in 
place. However, anything that I could think of could be really done, such
as adding more files to the playlist, could be easily circumnavigated
by exiting and quickly reentering the program with new files. Handling file
searching, playlist modification, and other behavior inside of the program
would require implementing fuzzy-finding, globbing, an actual tui library,
and in-program command input, which just seems like a lot of unneeded
complexity that I see no need for (unless people actually want it of course).

Some goals include.

- [] Actually have a CI/CD pipeline that would deploy, build and test code.
  This would make it much easier to download and run tmedia on other systems,
  as we could package tmedia in one folder with all shared libraries and
  runtime dependencies attached.
- [] Improve optimizations, cache performance, thread processing distribution,
  and decrease memory usage, perhaps through pooling of objects or using flatter
  data structures
- [] Use more development tools like code coverage, fuzzing
- [] Actually find a way to test tmedia reliably. As of right now, I have
  absolutely no idea how to test non-trivial sections of the code, such as
  audio playback and curses output, without just running the program. This
  severly limits the rate at which people can make non-trivial code, as the only
  way to verify if it works is to use the code for a long time (and that only
  verifies it on the tester's machine to be honest). If there's
  a way to test everything in integration automatically, It'd be priceless.

However, there are still a few features that I could see as far more than
quality-of-life

- [] Allow images to show like slideshows if there is other non-image media
  present in the current playlist. Currently, if images and playable media such
  as video and audio are in the same playlist, the image will block the playlist
  from continuing indefinitely until the user manually skips the current image.
  This makes it so when the user plays music from an album folder, they'd often
  have to enable ```--ignore-images``` or glob over the audio files specifically
  in order to avoid blocking images in the playlist.
  Adding a slideshow duration to images would make images work like videos
  and audio, which I could see simplifying the MediaFetcher class by making all 
  media "playable" media. However, whether this slideshow should be the default
  or not isn't very clear. A possibility would be to default images to run for
  an extremely long time, such as (DBL_MAX / 2) seconds, and only show the
  playback bar if the media's duration is under this amount. 


If there are any requests of anything to add, I'm completely open to access
them, and if you'd like to fork or contribute, go on ahead by all means.
