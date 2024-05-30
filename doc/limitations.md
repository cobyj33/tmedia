# limitations

Headers state the limitation, and often in "\>" blocks I explain why that
limitation exists or still exists. Unless you're actually wondering why
a certain limitation exists, these blocks are unnecessary to read. The headers
themselves give an 

If people request, I'll see what I can do about removing any of these
limitations, but the task as of right now far exceeds the reward.

## Only filesystem media files are allowed

> **DEV:**
>
> While, besides the std::filesystem::path usage, tmedia could theoretically 
> use the network to access media files, handling networking robustly 
> is something that is as foreign to me now as multimedia playback was for me
> when I started making tmedia, so I just decided to keep it as a non-goal.

## The media playlist cannot be changed at runtime

> **DEV:**
>
> In order to implement changing the media playlist, the user should be able
> to type paths or commands in some sort of field which will insert a media
> file or a group of in a specific index of the playlist. However, tmedia has 
> no notion of an input field or any real TUI widget as of now.
>
> Technically, if a media file is detected as invalid in some way when it is
> parsed, then the current media file could be removed from the playlist and
> playback could continue. So there are methods to remove and add to a playlist
> in code, but exposing it to users rudimentarily would leave out lots of
> expected behavior that the shell provides by default like filename matching
> and path autocompletion. A simple request turns hefty quickly, which is
> why it's been left unimplemented.

## The media playback cannot change speed

> **DEV:**
>
> Changing playback speed is surprisingly mathematically taxing, as just
> naively playing media back quicker results in a high squeaky
> Chipmunk-like audio and playing it back slower makes voices sound underwater.
> [Time scaling](https://en.wikipedia.org/wiki/Audio_time_stretching_and_pitch_scaling)
> without affecting pitch often requires computationally expensive operations like
> the FFT and STFT, and I definitely don't understand those concepts enough
> to implement them in a package I intend for others to use. 