# Is NCurses required?

No, any general curses implementation should work to compile tmedia.
I have made sure that no specific ncurses functionality is used anywhere within
the code for tmedia at all, so that building with other curses implementations
such as [PDCurses](https://github.com/wmcbrine/PDCurses) should be extremely
trivial, as in you just point to the installation of another curses
implementation with the cmake flag
[CMAKE_PREFIX_PATH](https://cmake.org/cmake/help/latest/envvar/CMAKE_PREFIX_PATH.html):

```bash
-DCMAKE_PREFIX_PATH=/path/to/curses/implementation
```

The linked-to version of the curses library will be printed out in cmake's
output when configuring the library, and can be found with
```tmedia --curses-version``` from the command line.

This is done with the hope that tmedia could one day run natively on a Windows
machine, as running through a virtual layer like WSL or VirtualBox comes with
an overhead that I'd rather not force users to endure on Windows machines.

(However, tmedia is definitely not near to handling Windows builds, as I have
literally no experience with building or configuring on the platform as of yet.)

## (RANT) Why curses? Aren't there more robust and modern TUI libraries.


ncurses can somewhat be a pain because of how primitive it still is, where it
mostly just generalises moving a cursor around the screen and printing
characters/escape codes instead of full-blown widgets and user interaction.
While there are a lot of other great TUI libraries that offer more descriptive
TUI design functionality than ncurses in C++, such as
[FTXUI](https://github.com/ArthurSonzogni/FTXUI) and
[finalcut](https://github.com/gansm/finalcut.git), I found that there were
more than a few reasons to choose ncurses over other libraries. 

- With ncurses there was more of a focus on speed and
  throughput when compared to "newer" TUI libraries in C and C++. Most
  TUI libraries seem to focus more on usability, but in turn opt in to a less
  flexible and performant API. Since tmedia must draw a image frame on the
  screen as fast as possible and the UI is simply a function of the current
  application state, any other TUI functionality is just unused bloat.
  Additionally, the ever-changing state of a media player lends more
  toward an immediate-mode interface, whereas a document or form would
  lean more toward a retained-mode interface that many TUI libraries opt for.
  
- ncurses is already extremely widely supported in unix package managers and
  distributions, which makes it really easy for anyone who would want to build
  tmedia to get hold of the library. It is also well tested in unix environments
  because of its ubiquity, so there's little worry for bugs on specific
  platforms that would past one's eye.

TLDR: tmedia is quite simple ui wise, I didn't want to make an entire app that
would be difficult to maintain and reason about, and also difficult to make
fast as a result.
