# Color

Color output is available for any terminal which supports colored output
and redefining colors, which is essentially any slightly modern terminal
emulator.

## NO_COLOR Environment Variable

tmedia supports turning off any color output to the terminal by
defining the NO_COLOR environment variable to any non-empty string as 
described by the [NO_COLOR](https://no-color.org/) proposal. If NO_COLOR
is set, changing between colored, gray, and normal modes will have no
effect.
