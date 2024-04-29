# tmedia TUI Design

This was the original first design document for the tmedia player tui. I decided
to add it to source control just as a way to document how the screen behaves.
**It's not completely accurate to how the screen is actually implemented**,
but most of the same elements are still present.

```txt
Some General Ideas

The arrows/(Next/Previous) signs at the top are only rendered if there is
actually a next or previous video queued up. For example, if the current media
is the last media and there is no looping. 

Looping, Playing, and Shuffling only apply to Video/Audio


Images strictly just go to the end or to the beginning, they don't loop at all


LINES < 10 or COLS 0-20 

Just Fullscreen

Breakpoint: Width 20+ COLS
    01. Palace
~~~~~~~~~~~~~~~~~~~

       VIDEO

~~~~~~~~~~~~~~~~~~~
   00:00 / 26:54

    Image3.png
~~~~~~~~~~~~~~~~~~~

      IMAGE

~~~~~~~~~~~~~~~~~~~



Breakpoint: Width 40+ COLS (40 COLS 12 LINES shown here)


<              01. Palace             >
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


       VIDEO/AUDIO VISUALIZATION


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
00:00 / 26:54 @@@----------------------
>          NL          NS          50%

Symbols:
> - PLAYING
|| - Paused
NL - Not Looped
R - Repeat
RO - Repeat One
NS - Not Shuffling
S - Shuffling
XX% - Volume %
M - Muted

<             Image4.png              >
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


                 IMAGE


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



Breakpoint: Width 80+ COLS (80 COLS 24 LINES shown here)

                                     01. Palace
< NONE                                                               02. Peso > 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~






                           VIDEO/AUDIO VISUALIZATION






~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
00:00 / 26:54 | 0@@@---1----2-----3-----4------5------6-----7------8----9------
PLAYING         NOT LOOPED           SHUFFLE                        VOLUME: 50% 

Words:
PLAYING
PAUSED

NO REPEAT
REPEAT
REPEAT ONE

NO SHUFFLE
SHUFFLE

VOLUME: XX%
VOLUME: MUTED

                                Image5.png
< Image4.png                                                    Image6.png    >
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~





                                     IMAGE





~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~




Breakpoint: Width 140+ COLS:

                                                                                                                                            
                                                                01. Palace                                                                  
Previous: NONE                                                                                                               Next: 02. Peso
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~







                                                            VIDEO/AUDIO VISUALIZATION








~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
00:00 / 26:54 | 0@@@-------1-----------2----------3---------4---------5---------6------------7----------8-------------9--------------------
|PLAYING (SPACE)|         |NOT LOOPED (L)|           |NO SHUFFLE (S)|               |VOLUME: 50% (UP/DOWN)|                      |M: More|
 






                                                               Image4.png 
Previous: Image3.png                                                                                                      Next: Image5.png
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~







                                                                 IMAGE








~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                |OUTPUT: COLORED BACKGROUND (C/B/G)|                                          |F: Toggle Fullscreen|
```txt