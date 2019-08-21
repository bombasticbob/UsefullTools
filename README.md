# UsefullTools (ok it's spelled wrong, so what)

A bunch of tools, shell scripts, POSIX utilities, and other things I like to use

I put them here in case anyone else in the universe thinks they're useful, too

here's a short list of what they do

* read.fuses.sh - this one uses an AVRISPMkII (or clone) to read all of the fuses on an atmega
CPU, allowing you to specify which one if it's not an atmega328p .  It also
decodes them into human readable form which is the important part.

* new.write.fuses.sh - a simple script to write fuses for atmega series using AVRISPMkII

* flash2mp3.sh - extract mp3 audio from flash video, or actually ANY video format that ffmpeg
will recognize.  Originally designed for flash videos downloaded from sites
like youtube.  Uses 'twolame' to encode.

* mpeg2mp4.sh - create h.264 video as 'mp4' from any video source recognizable by ffmpeg

* agg.c - A simple POSIX utility that allows you to aggregate column data, separated
by white space.  In short, you can do sums, averages, and so on, with
text data formatted in columns (text or numeric).  Run the program with '-h'
to get a simple 'usage' display of options.

* do_dft.c, do_dft2.c - These programs do a DFT analysis of x,y data (2 columns) with
arbitrary time between data points. The first one is a basic DFT, single-threaded.
The 2nd one (do_dft2) has some compile options for "fast DFT" (using fast sin/cos
algorithms that work well on embedded or microcontrollers) and is also multi-threaded.
They work with text formatted in columns separated by white space.  Run the programs
with '-h' to get a list of options, etc.

* banner.sh - generates a 'figlet' banner using 'figlet', with some copyright info.
It is basically designed to be a block header for C/C++ programs, but you could adapt
it for shell scripts, python, java(script), whatever, as you see fit.  Just edit the
line that says 'Your Organization' and put, well, YOUR Organization in there.

(I'll add more at some point)

