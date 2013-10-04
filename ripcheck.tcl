#!/usr/local/bin/tclsh8.6

# to use:
#
# tclsh8.6 ripcheck.tcl <wavfiles>
#
# available from:
# http://magnatune.com/info/ripcheck
#
# written by John Buckman of Magnatune
#
# Released under GPL v3 license
# http://www.gnu.org/licenses/gpl.html
#
# You will need Tcl/Tk v8.6 or newer, which can be downloaded from ActiveState
#
# If a large number of identical audio samples are found in the WAV file, these
# are assumed to be rip defects and the position where those occured is displayed.
# A GIF file is also made
#
# This program requires "ripcheckc" to be compiled as a binary
# in the current directory. If you don't want this, uncomment
# the line "return 1" in proc ctest {fname}.
#
# Also requires "imagemagick" to be installed in order to resize and antialias
# the GIF file which is created.  The program which is run is called "convert"
#
# If you get an error about no DISPLAY , try to run "export DISPLAY=0:0" in your bash shell before running this program

# hack to make this work w/o an xterm opening
#set ::env(DISPLAY) 0:0
#package require Tcl 8.6
package require Tk
 
proc gif_write {gifname width height filename palette image} {

    image create photo fooble -width $width -height $height
    fooble blank
    for { set x 0 } { $x < [llength $image] } { incr x } {
		set data [lindex $image $x]
		
        for { set y 0 } { $y < [llength $data] } { incr y } {
			set i [lindex $data $y]
			fooble put \#[lindex $palette $i] -to $y $x
        }
    }
    grid [label .test -image fooble]
	
	set wavgif1 "${gifname}_tmp.gif"
	#set wavgif2 "wav.gif"
	set wavgif2 $gifname
	file delete -- $wavgif1
	file delete -- $wavgif2
    fooble write $wavgif1 -format gif
	set newwidth [expr {$width / 4}]
	exec convert -flip -resize ${newwidth}x${height}! $wavgif1 $wavgif2
	file delete -- $wavgif1
	destroy .test
	destroy fooble
}


proc write_file {filename data} {
    #puts "Writing $filename"
    set fn [open $filename w]
    puts $fn $data 
    close $fn
    return 1
}

proc comma { num } {
	while {[regsub {^([-+]?[0-9]+)([0-9][0-9][0-9])} $num {\1,\2} num]} {}
	return $num
}

##+##########################################################################
#
# fmtChunk -- parses and dumps the fmt chunk
#
proc fmtChunk {fname magic len} {
	global bytesSec scanFmt fin
	
	binary scan [read $fin 16] $scanFmt(fmt) \
	fmt chan sampleSec bytesSec align bitsSample
	
	set line " fmt: $fname - $magic - $fmt "
	append line [expr {$chan == 1 ? "mono" : $chan == 2 ? "stereo" : "$chan channels"}]
	append line " [comma $sampleSec]Hz "
	append line "[comma $bytesSec] bytes/sec "
	append line "[comma $align] bytes/sample "
	append line "[comma $bitsSample] bits/sample "
	
	if {$fmt != 1} {                            ;# Extra header stuff
		binary scan [read $fin 2] $scanFmt(16) extra
		append line "extra: $extra"
	}
	
	return $line
}


##+##########################################################################
#
# DoFile -- Parse one wave file
#
proc DoFile {fname} {
#	puts "Processing file: $fname"
	global fin scanFmt fmtScan
	global bytesSec
	
	set dupel 0
	set duper 0
	set oldleft 0
	set oldleft1 0
	set oldleft2 0
	set oldleft3 0
	set oldleft4 0
	set oldleft5 0
	set oldright 0
	file delete "wav.gif"
	
	#puts $fname
	set fin [open $fname "r"]
	fconfigure $fin -translation binary
	
	set magic [read $fin 4]
	
	array set scanFmt {16 s 32 i fmt "ssiiss"}  ;# Little endian by default
	set scanformat "s*"
	if {$magic eq "RIFX"} {                     ;# This is big-endian format
		puts "$fname - big-endian format"
		array set scanFmt {16 S 32 I fmt "SSIISS"}
		set scanformat "S*"
	} elseif {$magic ne "RIFF"} {
		puts "$fname - Bad magic number: '$magic'"
		exit
	} 
	
	#puts "magic: $magic"
	
	binary scan [read $fin 4] $scanFmt(32) left  ;# Should be file length - 8
	set type [read $fin 4]
	
	if {$type ne "WAVE"} {
		puts "Not a WAVE file: '$type'"
		exit
	}
	
	set dataLen 0
	set maxdupes 200
	set totalreadcnt 0
	set readcnt 0
	set maxl 0
	set minl 0
	set maxr 0
	set minr 0
	set pngleft {}
	set currentleft 0
	set currentright 0
	set imageheight 70
	#set divisor [expr {65536 / $imageheight}]

	set middle [expr {$imageheight / 2}]
	set leftvals {}
	set warnings ""
	
	set graphiccounter 0
	set prevbadspot 0
	set make_png 1
	set badl_count 0
	regsub {\.[Ww][Aa][Vv]$} $fname {.gif} gifname
	if {$gifname == $fname} {
		puts "Warning, wav filename didn't create a valid gif filename, so sending output to wav.gif"
		set gifname "wav.gif"
	}
	file delete $gifname
	
	regsub {\.[Ww][Aa][Vv]$} $fname {.txt} txtname
	if {$txtname == $fname} {
		puts "Warning, wav filename didn't create a valid txt filename, so sending text output to errors.txt"
		set txtname "errors.txt"
	}
	file delete $txtname
	set poploc 0

	while {1} {                                 ;# Loop through every chunk
		set chunk [read $fin 4]                 ;# Chunk type
		if {[eof $fin]} break
		binary scan [read $fin 4] $scanFmt(32) len ;# Chunk length
		set eoc [expr {[tell $fin] + $len}]     ;# End of chunk
		set duration [expr {round(($len/4) / 44.1)}]
		set duration_almost_at_end [expr {round(($len/4) / 44.1) - 5000}]
		
		if {[string trim $chunk] == "fmt"} {
			set chunkinfo [fmtChunk [file tail $fname] $magic $len]
		} else {

			if {$chunk eq "data"} {             ;# Possibly multiple data chunks
				incr dataLen $len
			}
			set samples [expr {$len/4}]
			set cnt 0		
			while {$readcnt <= $len} {
				set chunk [read $fin 10240]                 ;# Chunk type
				binary scan $chunk $scanformat parts
				if {[eof $fin]} break
				foreach {left right  } $parts {
					incr readcnt 4
				
					if {$left < $minl}  { set minl $left }
					if {$left > $maxl}  { set maxl $left }
					
					if {$right < $minr}  { set minr $right }
					if {$right > $maxr}  { set maxr $right }

					incr cnt 1
					set duration [expr {10*int(100*(double($readcnt) / $::bytesSec))}]

					lappend windowleft $left
					if {[llength $windowleft] > 5000} {
						# trim the sample window when it's over 1000
						#puts "trimming leftval window"
						set windowleft [lrange $windowleft 50 999999]
					}
					
					# look for a pop 
					if {$oldleft5 == 0 && $oldleft4 == 0 && $oldleft3 == 0 && $oldleft2 == 0 && ($oldleft1 > 10000 || $oldleft1 < -10000) && $cnt > 12 && ($duration < $duration_almost_at_end)} {
						set warning "- possible pop found at sample count $cnt ($duration millisecs) values: '$oldleft5, $oldleft4, $oldleft3, $oldleft2, $oldleft1, $oldleft, $left'"
						lappend warnings $warning
						set poploc $cnt
						
						# add to the graphic, the first 5 bad spots
						set chunk [read $fin 5120]                 ;# Chunk type
						binary scan $chunk $scanformat parts
						set aftererror {}
						foreach {left1 right1} $parts {
							incr readcnt 4
							incr cnt 1
							lappend aftererror $left1
						}
						
						set pngleft [concat $pngleft $windowleft 999999 [lrepeat 20 $oldleft5] [lrepeat 20 $oldleft4] [lrepeat 20 $oldleft3] [lrepeat 20 $oldleft2] [lrepeat 20 $oldleft1] [lrepeat 20 $oldleft] [lrepeat 20 $left] 999999 $aftererror [lrepeat 50 999998] ]
					}
		
					# look for a dropped sample, but not closer than 8 samples to the previous pop 
					if { (($oldleft2 > 20000 && $oldleft1 == 0 && $oldleft > 20000) || ($oldleft2 < -20000 && $oldleft1 == 0 && $oldleft < -20000)) && $cnt > 12 && ($cnt > $poploc + 8) && ($duration < $duration_almost_at_end)} {
						set warning "- possible dropped sample found at sample count $cnt ($duration millisecs) values: '$oldleft2, $oldleft1, $oldleft'"
						lappend warnings $warning

						set poploc $cnt
						
						# add to the graphic, the first 5 bad spots
						set chunk [read $fin 5120]                 ;# Chunk type
						binary scan $chunk $scanformat parts
						set aftererror {}
						foreach {left1 right1} $parts {
							incr readcnt 4
							incr cnt 1
							lappend aftererror $left1
						}
						
						set pngleft [concat $pngleft $windowleft 999999 [lrepeat 20 $oldleft5] [lrepeat 20 $oldleft4] [lrepeat 20 $oldleft3] [lrepeat 20 $oldleft2] [lrepeat 20 $oldleft1] [lrepeat 20 $oldleft] [lrepeat 20 $left] 999999 $aftererror  [lrepeat 50 999998] ]

					}
					
					#set warnings ""
					set badl 0
					set badr 0
					if {$oldleft == $left} {
						incr dupel
					} else {
		
		
						if {($dupel > $maxdupes) && ($duration > [expr {$prevbadspot + 5000}])} {
							if {$duration > 5000 && $duration < $duration_almost_at_end} {
							# ignore empty space at the beginning of the sound file
								set warning "- [expr {2*$dupel}] duplicate samples found (value: $oldleft) in the left channel at sample '[comma $cnt]' ($duration milliseconds)"
								#puts $warning
								lappend warnings $warning
								#set stereowarning "warning $dupel duplicate samples found in both channels at sample '[comma $cnt]' : at $duration milliseconds"
								set badl 1
								if {[llength $warnings] < 5} {
									if {$oldleft < 0 && $oldleft > -10} {
										# do nothing if values are tiny, as this is probably a fade-out
									} elseif {$oldleft > 0 && $oldleft < 10} {
										# do nothing if values are tiny, as this is probably a fade-out
									} else {
										
										# add to the graphic, the first 5 bad spots
										set chunk [read $fin 10240]                 ;# Chunk type
										binary scan $chunk $scanformat parts
										set aftererror {}
										foreach {left right } $parts {
											incr readcnt 4
											lappend aftererror $left
										}
										
										set pngleft [concat $pngleft $windowleft 999999 [lrepeat 100 $oldleft] 999999 $aftererror [lrepeat 50 999998] ]
										#set windowleft {}
										set prevbadspot $duration
									}
								}
							}
						}
						set dupel 0
					}
					
					
					set currentleft 0
					set currentright 0
					
					set oldleft5 $oldleft4
					set oldleft4 $oldleft3
					set oldleft3 $oldleft2
					set oldleft2 $oldleft1
					set oldleft1 $oldleft
					set oldleft $left
					set oldright $right
					
					
				}
				if {[eof $fin]} break
			}
		}
		seek $fin $eoc start
	}
	close $fin

	set maxpng 2
	set minpng 2
	for {set x 0} {$x < [llength $pngleft]} {incr x} {
		set in [lindex $pngleft $x]
		if {$in > $maxpng && $in < 999990}  { set maxpng $in}
		if {$in < $minpng}  { set minpng $in}
	}
	
	if {[llength $warnings] > 0} {
		puts "Analyzing file: $fname"
		#puts $chunkinfo
		puts [join $warnings \n]
		puts ---
		write_file $txtname "[join $warnings \n]"
	}
	
#	set max $maxpng
#	if {[expr {-1*$minpng}] > $max} {
#		set max [expr {-1*$minpng}]
	#}
	#set divisor [expr {$max / $imageheight}]
	set maxpng [expr {int(1.1 * ($maxpng + (-1*$minpng)))}]
	set divisor [expr {2 * ($maxpng / $imageheight)}]
	if {$divisor == 0} {
		# double the size of tiny waveforms (whose values are less than 75)
		set divisor .5
	}

	set debug 0
	if {$debug == 1} {
		if {[llength $pngleft] > 0} {
			puts "max right: $maxr"
			puts "min right: $minr"
			puts "max left: $maxl"
			puts "min left: $minl"
			puts "image width: [llength $pngleft]"
			puts "length: [comma $samples] samples, [comma $duration] milliseconds"
			puts "Max chart point: $maxpng / min: $minpng "
			puts "divisor: $divisor"
		}
	}

#set divisor 2
	
	#set maxi $maxpng
	#if {[expr {-1 * $minpng}] > $maxpng} {
	#	set maxpng [expr {-1 * $minpng}]
	#}
	#puts "offset: $maxpng"

	if {($make_png == 1) & ([llength $pngleft] > 0)} {
		#puts "Making graphic now: $gifname\n---"
		set palette {FFFFFF 0000FF FF0000 BBBBBB EEEEEE}
		set image {}
		set inbad 0
		for {set y 0} {$y < $imageheight} {incr y} {
			set row {}
			for {set x 0} {$x < [llength $pngleft]} {incr x} {
				set in [lindex $pngleft $x]
				set currentvalue [expr {int(($in / $divisor) + ($imageheight/2))}]
				if {$in == 999999} {
					#lappend row 2
					if {$inbad == 0} {
						set inbad 1
					} else {
						set inbad 0
					}
				} elseif {$in == 999998} {
					lappend row 4
				} elseif {$currentvalue == $y || [expr {1+$currentvalue}] == $y || [expr {-1+$currentvalue}] == $y} {
					if {$inbad == 0} {
						lappend row 1
					} else {
						lappend row 2
					}
				} elseif {$y == $middle} {
					lappend row 3
				} else {
					lappend row 0
				}
			}
			lappend image $row
		}
		gif_write $gifname [llength $pngleft] $imageheight "text.gif" $palette $image
	}
}

# quick version of this tcl script, runs as C code, to see if a more detailed analysis
# of this WAV file would make sense. 
proc ctest {fname} {
	# uncomment this if you don't have the ripcheckc program compiled for your platform
	#return 1
	set results [exec /m/admin/ripcheckc $fname 2>@1]
	puts -nonewline "Quick test: $fname"
	if {[string first "everything ok" $results] == -1} {
		puts "\n----\nFound potential WAV file CD ripping defects in\n$fname\nanalyzing further and making a GIF file of the problematic areas, with a corresponding TXT file.\n"
		#puts "results: '$results'"
		# of "everything ok" is not in the result set, then run the test manually
		return 1
	}
	puts "everything ok"
	return 0
}

################################################################
################################################################

if {$argc == 0} {
	puts stderr "usage: \n\nripcheck.tcl <wave files>\n\nor visit http://magnatune.com/info/ripcheck for more information.\n\n"
	exit
}

foreach arg $argv {
	set arg [file normalize $arg]               ;# Make ok for glob
	foreach fname [glob -nocomplain $arg] {     ;# Do wildcard expansion if any
		set candidate [ctest $fname]
		if {$candidate != 0} {
			DoFile $fname
		}
	}
}
exit


