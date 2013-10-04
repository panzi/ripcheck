ripcheck
========
"ripcheck" runs a variety of tests on a WAV file, to see if there are
potential mistakes that occurred in converting a CD to a WAV file.

I've made this program available as free & open source, and it can be
download at [Sourceforce](https://sourceforge.net/projects/ripcheck/).

You can use this program on your own WAV files to see if there are
potential defects in your CD rips.

More documentation (and discussion) is available at
[blog.magnatune.com](http://blog.magnatune.com/2013/09/ripcheck-detect-defects-in-cd-rips.html).

WHAT?
-----
 * ripcheckc.c - source code
 * ripcheckc - Mac OSX command line binary
 * ripcheckc.exe - Windows command line binary
 * ripcheck.tcl - Slower version, creates pretty graphics of problem areas
 * README.TXT - this info file

Released under [GPL v3 license](http://www.gnu.org/licenses/gpl.html)

WHY?
----
This program was written because we'd received some complaints of
occasional "pops" at the beginning on some albums at Magnatune.
Further research found that most of the albums we released in 2007 had
various CD ripping problems.  We also found various CDRs burned by our
musicians that had small defects in them, probably due to cheap CDR
media being used, or the CDR being burnt too quickly.

Back in 2007, we were using an early version of the open source
windows program [CDEX](http://cdexos.sourceforge.net/) and on many albums
from that time this program would introduce a very short "click" sound
at the beginning of each WAV file, which would then trickle down to
all the file formats we offered at Magnatune. Unfortunately, we didn't
notice this at the time. Most people didn't notice it either, probably
assuming it was an audio streaming glitch, since it was so short.

In order to discover just how bad the problem might be, I wrote a
program called "ripcheck", because, to my surprise, no program
currently exists to try to determine if there might be CD ripping
errors in an audio file.

WHAT KINDS OF PROBLEMS?
-----------------------
Besides the common problem short click at the beginning of a track,
this program also discovers:

 1. the occasional dropped sample (i.e. a sample value of zero) in the
    middle of a song. This might be audible as a click as well.

 2. the occasional repeated sample value, usually for about 1/1000th of
    second. This would be audible as a very short tone. 

 3. the occasional empty spot in the middle of a song.

All in all, my program found that 124 albums at Magnatune had some
sort of defect in them.

Most of the defects could be attributed to our early years, when we
were running our ripping on Windows with software that was in its
early versions (remember, we've been doing this since 2003, when CD
ripping was pretty new).

Occasional problems also crept in from CDs burnt too quickly or on
cheap CDR media, that were then sent to us as "masters".

What's amazing to me is that all these bad RIPs were sent by us to
Amazon and iTunes, and those services never noticed anything.
Obviously, though companies don't have any sort of "ripcheck" program,
to do automatic quality control on the audio they distribute.

All our releases are now automatically tested with "ripcheck" before
we make them available.

HOW TO OBTAIN:
--------------
If you'd like to run this program yourself, on your own music library,
please download at [Sourceforce](https://sourceforge.net/projects/ripcheck/).

It's a command line program that works on Windows, Mac and Unix/Linux.
There is a fast version, which shows potential problems as text, and a
much slower version which generates graphics of the WAV file defects,
so you can easily see where the problem is.

In all cases, the location of the glitch is given in samples and
microseconds. You can use an audio editing program to type this number
directly in and jump to that point.

The program is intentionally over broad in detecting problems, and
will often yield false positives (about half of the time, in my
experience). For example, if you have electronic music which toys with
audio glitches, perhaps creating pure tones with the same sample
value, you might get a lot of false positives on that kind of music.

My goal was to always detect potential problems and never overlook
real problems. On the Magnatune music collection, I personally
listened to every audio file that had a warning, and viewed the
samples in an audio editor, before marking a warning as a false
positive.

The source code is pretty simple to read, and I'd love for any
programmers who find it useful to devise other tests and add them to
the list of things to detect.

HOW TO RUN:
-----------
To run it yourself, type:

	ripcheckc <wavfilename>

Only WAV files are handled, and they must be 44k/16bit.

If you downloaded the windows .exe version of this program, you will 
need to install [cygwin](http://www.cygwin.com/).

If you want to use this program on Unix/Linux, you will need to
compile the program with this command line:

	gcc ripcheckc.c -o ripcheckc

The ripcheck.tcl program requires Tcl/Tk to be on your system, and
ImageMagick as well. It runs about 3x slower, but creates attractive
graphics of the WAV file problem areas. This can be a great time saver
if you have a lot of problematic WAV files, aiding you in quickly
reviewing the problems visually to decide if they are false positives.

\- John Buckman <john@magnatune.com>
