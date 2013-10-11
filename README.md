ripcheck
========
"ripcheck" runs a variety of tests on WAV files, to see if there are
potential mistakes that occurred in converting a CD to a WAV file.

You can use this program on your own WAV files to see if there are
potential defects in your CD rips.

More documentation (and discussion) is available at
[blog.magnatune.com](http://blog.magnatune.com/2013/09/ripcheck-detect-defects-in-cd-rips.html).


License
-------
See COPYING.txt for the [GPL v3 license](http://www.gnu.org/licenses/gpl-3.0.html)

Why?
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

In order to discover just how bad the problem might be, I (John Buckman)
wrote a program called "ripcheck", because, to my surprise, no program
currently exists to try to determine if there might be CD ripping
errors in an audio file.

What kinds of problems?
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

How to obtain
-------------
The original version of this program can be found at
[Sourceforce](https://sourceforge.net/projects/ripcheck/). This fork
however can be found at [GitHub](https://github.com/panzi/ripcheck).

It's a command line program that works on Windows, Mac and Unix/Linux.
It prints potential problems as text, and a if it is compiled with
libpng support it can also generate graphics of the WAV file defects,
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

Usage
-----
To run it yourself, type:

    ripcheck [OPTIONS] [WAVE-FILE]...

Only PCM WAV files are supported.

### Options

	-h, --help                    print this help message
	-v, --version                 print version information
	-V, --visualize[=PARAMS]      print wave forms around found problems to PNG images
	                              PARAMS is a comma separated list of key-value pairs that
	                              define the size and color of the generated images.
	
	                              samp-width=PIXELS      width of a sample (default: 20)
	                              samp-height=PIXELS     height of a samle above the zero line
	                                                     (default: 50)
	                              bg-color=COLOR         background color (default: #FFFFFF)
	                              wave-color=COLOR       color of the wave form (default: #2084FF)
	                              zero-color=COLOR       color of the zero line (default: #7F7F7F)
	                              error-color=COLOR      color of the error sample (default: #FF2020)
	                              error-bg-color=COLOR   background color of the error sample
	                                                     (default: #FFC440)
	
	                              COLOR may be a HTML like hexadecimal color string (e.g. #FFFFFF)
	                              or one of the 16 defined HTML color names (e.g. white).
	
	    --image-filename=PATTERN  use PATTERN for the names of the generated image files
	                              Patterns can reference certain variables using {VARNAME}.
	                              In order to put a { or } in the resulting filename write {{ or }}.
	
	                              errorname           'pop', 'drop' or 'dupes'
	                              filename            name of the WAV file without path
	                              basename            name of the WAV file without path or extension
	                              filepath            path of the WAV file
	                              dirname             path of the directory of the WAV file
	                              channel             channel in which the error happened
	                              first_error_sample  first sample in this error
	                              last_error_sample   last sample in this error
	                              error_samples       number of samples in this error
	                              first_window_sample first sample in current window
	                              last_window_sample  last sample in current window
	                              window_size         number of samples in window
	
	                              If the error happened at the very beginning of the WAV file then
	                              window_size might be bigger than what last_window_sample and
	                              first_window_sample imply.
	
	-t, --max-time=TIME           stop analyzing at TIME
	-b, --max-bad-areas=COUNT     stop analyzing after COUNT problems found
	-i, --intro-length=TIME       start analyzing at TIME (default: 5 sec)
	-o, --outro-length=TIME       stop analyzing at TIME before end (default: 5 sec)
	-p, --pop-limit=VOLUME        set the minimum volume of a pop to VOLUME (default: 33.333 %)
	-d, --drop-limit=VOLUME       set the minimum volume of samples around a drop to VOLUME
	                              (default: 66.666 %)
	    --pop-drop-dist=TIME      ignore drops before TIME after a pop (default: 8 sampels)
	-u, --dupe-limit=VOLUME       ignore dupes more silent than VOLUME (default: 0.033 %)
	    --min-dupes=COUNT         set the minimum repetiton of the same sample that is
	                              recognized as a dupe to COUNT (default: 400)
	    --dupe-dist=TIME          ignore dupes that follow closer than TIME to another dupe
	                              (default: 1 sample)
	-w, --window-size=COUNT       print COUNT samples when a problem is found (minimum: 7)
	                              Even if COUNT is bigger ripcheck does not use more than 7
	                              samples at a time for detecting problems. (default: 7)

### Units

    TIME
      TIME values can be given in samples, seconds or milliseconds.
      Examples: 400 samp, 5 sec, 4320 msec

      samp, (node) ... samples
      sec, s ......... seconds
      msec, ms ....... millieseconds

    VOLUME
      VOLUME values can be given in bit rate dependant values or percentages.
      Examples: 32000, 33.33 %

      (node) ... bit rate dependant absolute volume
      % ........ percentage of maximum possible volume

Build
-----
This program uses [cmake](http://www.cmake.org/) to compile:

    make build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make -j2
    sudo make install

If you don't have libpng or don't want to build visualization support
use this cmake line:

    cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DWITH_VISUALIZE=OFF

\- John Buckman <john@magnatune.com> (original version)  
\- Mathias Panzenböck (this fork)
