/*********************************************************************
    ripcheckc.c
    
    Usage: ripcheckc filename.wav
         
    to compile:
    gcc ripcheckc.c -o ripcheckc

    from:
    http://blog.magnatune.com/2013/09/ripcheck-detect-defects-in-cd-rips.html

    written by John Buckman of Magnatune

    Released under GPL v3 license
    http://www.gnu.org/licenses/gpl.html
         
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

long samples_to_output = -1;

unsigned char *get_bytes(FILE *f, int n)
{
    static unsigned char s[16];

    assert (n <= sizeof s);
    if (fread(s, n, 1, f) != 1) {
        fprintf(stderr, "Read error\n");
        exit(1);
    }
    return s;
}

unsigned long get_ulong(FILE *f)
{
    unsigned char *s = get_bytes(f, 4);
    return s[0] + 256LU * (s[1] + 256LU * (s[2] + 256LU * s[3]));
}

unsigned get_ushort(FILE *f)
{
    unsigned char *s = get_bytes(f, 2);
    return s[0] + 256U * s[1];
}

void dumpwave(FILE *f, char* fn)
{
    signed int i, x, x1, x2, x3, x4, x5, x6, channels, bits = 0;
    signed int dupecount = 0;
    unsigned long millisecs = 0;
    unsigned long len;
    unsigned long sample = 0;
    unsigned long count;
    unsigned long duration = 0;
    unsigned long almost_end = 0;
    unsigned char s[5];
    unsigned long duration_bytes = 0;
    unsigned int bad_areas = 0;
    unsigned long prevbadspot = 0;
    unsigned long silence_samples_before_start = 0;
    unsigned long silence_samples_after_start = 0;
    unsigned long nonsilence_samples_after_start = 0;
    unsigned short pops = 0;
    unsigned dtype = 0;
    unsigned long poploc = 0;
    unsigned int debug = 0;
    unsigned long sampling_rate = 0;
    unsigned long bytes_per_second = 0;
    unsigned short bytes_times_channels = 0;
    char chartmp = 0;

    if (memcmp(get_bytes(f, 4), "RIFF", 4) != 0) {
        fprintf(stderr, "Not a 'RIFF' format\n");
        return;
    }
    duration_bytes = get_ulong(f);
    if (debug == 1) {
        fprintf(stderr, "[RIFF] (%lu bytes)\n", duration_bytes);
    }
    if (memcmp(get_bytes(f, 8), "WAVEfmt ", 8) != 0) {
        fprintf(stderr, "Not a 'WAVEfmt ' format\n");
        return;
    }
    len = get_ulong(f);
    
        
    if (debug == 1) {
        fprintf(stderr, "[WAVEfmt ] (%lu bytes)\n", len);
    }
    duration = ((1000.0*duration_bytes) / 44100.0) / 4.0;
    almost_end = duration - 5000;
    dtype = get_ushort(f);
    if (debug == 1) {
        fprintf(stderr, "Duration %lu milliseconds\n", duration);
        fprintf(stderr, "  Data type = %u (1 = PCM)\n", dtype);
    }
    channels = get_ushort(f);
    sampling_rate = get_ulong(f);
    bytes_per_second = get_ulong(f);
    bytes_times_channels = get_ushort(f);
    if (debug == 1) {
        fprintf(stderr, "  Number of channels = %u (1 = mono, 2 = stereo)\n", channels);
        fprintf(stderr, "  Sampling rate = %luHz\n", sampling_rate);
        fprintf(stderr, "  Bytes / second = %lu\n", bytes_per_second);
        fprintf(stderr, "  Bytes x channels = %u\n", bytes_times_channels);
    }
    bits = get_ushort(f);
    
    if (debug == 1) {
        fprintf(stderr, "  Bits / sample = %u\n", bits);
    }
    
    if (debug == 1) {
        for (i = 16; i < len; i++) {
            fprintf(stderr, "%02x ", fgetc(f));
        }
        fprintf(stderr, "\n");
    }
    dupecount = 0;
    while (fread(s, 4, 1, f) == 1) {
        len = get_ulong(f);
        s[4] = 0;
        if (debug == 1) {
            fprintf(stderr, "[%s] (%lu bytes)\n", s, len);
        }
        if (memcmp(s, "data", 4) == 0) break;
        for (i = 0; i < len; i++) {
            chartmp = fgetc(f);
            if (debug == 1) {
                fprintf(stderr, "%02x ", chartmp);
            }
        }
        fprintf(stderr, "\n");
    }
    for (count = 0; count != samples_to_output; count++) {
        for (i = 0; i < channels; i++) {
            if (bits <= 8) {
                if ((x = fgetc(f)) == EOF) {
                    if (bad_areas == 0) {
                        printf("everything ok - %s\n", fn); 
                    };
                    return;
                }
                x -= 128;
            } else {
                if (fread(s, 4, 1, f) != 1) {
                    if (bad_areas == 0) {
                        printf("everything ok - %s\n", fn); 
                    };
                    return;
                };
                x = (short)(s[0] + 256 * s[1]);
            }
            /*printf("%d ", x);*/
            
            sample = count * 2;
            millisecs = ((1000*sample) / 44100);

            /* look for a pop */
            if (x6 == 0 && x5 == 0 && x4 == 0 && x3 == 0 && (x2 > 10000 || x2 < -10000) && count > 12 && (millisecs < almost_end)) {
                printf("- possible pop found at sample count %lu (%lu millisecs) values: '%i, %i, %i, %i, %i, %i, %i' %s\n", sample, millisecs, x6, x5, x4, x3, x2, x1, x, fn);
                bad_areas++;
                poploc = x;
            }

            /* look for a dropped sample, but not closer than 8 samples to the previous pop */
            if ( ((x2 > 20000 && x1 == 0 && x > 20000) || (x2 < -20000 && x1 == 0 && x < -20000)) && count > 12 && (count > poploc + 8) && (millisecs < almost_end)) {
                printf("- possible dropped sample found at sample count %lu (%lu millisecs) values: '%i, %i, %i' %s\n", sample, millisecs, x2, x1, x, fn);
                poploc = x;
                bad_areas++;
            }
            
            if (x1 == x) {
                dupecount++;
            } else {
                if (x != 0 && (dupecount > 400) && (millisecs > 5000) && (millisecs < almost_end) && (millisecs > (prevbadspot + 5000))) {
                    if (x < 0 && x > -10) {
                        /* do nothing */
                    } else if (x > 0 && x < 10) {
                        /* do nothing */
                    } else {
                        printf("- %d dupes found at sample count %lu (%lu millisecs) value='%i' %s\n", dupecount, sample, millisecs, x, fn);
                        bad_areas++;
                        prevbadspot = millisecs;
                    }
                }
                dupecount = 0;
            }
            
            x6 = x5;
            x5 = x4;
            x4 = x3;
            x3 = x2;
            x2 = x1;
            x1 = x;
            /* if (i != channels - 1) printf("\t");*/
        }
        /* printf("\n"); */
    }
    
    if (bad_areas == 0) {
        printf("everything ok\n"); 
    }
}

int main(int argc, char *argv[])
{
    int i;
    FILE *f;

    if (argc == 1) {
        printf("Please provide a list of .wav files to test.\n\nor visit http://magnatune.com/info/ripcheck for more information.\n\n");
    }

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            samples_to_output = atol(&argv[i][1]);
        } else if ((f = fopen(argv[i], "rb")) != NULL) {
            dumpwave(f, argv[i]);  fclose(f);
        } else {
            fprintf(stderr, "Can't open %s\n", argv[i]);
        }
    }
    return 0;
}

