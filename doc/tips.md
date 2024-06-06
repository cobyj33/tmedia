# Tips

## How can I continue to play music outside of a terminal session.

I'd recommend using a screen multiplexer like
[tmux](https://github.com/tmux/tmux/wiki), which allows terminals to be
run in the background (among many other things).

I use it often whenever I want music off my filesystem
to play in the background, and then I could reattach to the terminal at anytime
and input commands into tmedia if I'd like.

The audio continues to playback without the terminal screen opened,
and I've even found that the audio continues to playback smoothly even
with the terminal window completely closed, as long as the tmux server
continues to run.

## How can I automatically run a configuration of options anytime I start tmedia

I'd reccomend using a
[shell alias](https://www.gnu.org/software/bash/manual/bash.html#Aliases)
for automatically expanding tmedia to whatever
commands you'd like it to use.

For example, I personally enjoy these
shell aliases to use whenever listening to music, as it starts up tmedia
with lower volume, color enabled, background enabled,
and only the audio stream enabled.

```bash
alias tmusic="tmedia --disable-video-stream --volume 10% -cb --ignore-images"
```

If a shell alias can't be used, such as if your shell does not support
aliases, then a shell function can be equally used,
although with a slightly different syntax:

```bash
tmusic() {
  tmedia --disable-video-stream --volume 10% -cb --ignore-images "$@"
  # The "$@" inserts all arguments sent to tmusic to the actual tmedia
  # program:
  # https://www.gnu.org/software/bash/manual/bash.html#Special-Parameters
}
```


