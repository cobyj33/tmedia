Valid Options:

-h, --help
-v, --version

-c, --color, --colored

-g, --gray, --grayscale, --grey, --greyscale

-b, --background

  
--refresh-rate POSITIVE_INTEGER


--ma-backend MA_BACKEND
  Valid options are: wasapi, dsound, winmm, coreaudio, alsa, pulseaudio, jack, sndio, audio4, oss, aaudio, opensl, webaudio, null

--ignore-images
--ignore-videos
--ignore-audio, --ignore-audios
:ignore-images
:ignore-videos
:ignore-audio, :ignore-audios

--no-audio
--volume
-m, --mute, --muted

-l, --loop 
-s, --shuffle

--chars

-r, --recursive
:r, :recursive
:nr, :no-recursive, :not-recursive

--fullscreen

-d, --dump

--curses-version
--ffmpeg-version

tmedia ~/Music --ignore-images --no-videos

tmedia ~/Music --ignore-images --omit ~/Music/Videos

tmedia ~/Music --no-audio
