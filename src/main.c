/**
 *  ripcheck - find potential ripping errors in WAV files
 *  Copyright (C) 2013  John Buckman of Magnatune, Mathias Panzenböck
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <getopt.h>
#include <errno.h>

#include "ripcheck.h"
#include "print_text.h"

#ifdef WITH_VISUALIZE
#include "print_image.h"
#endif

const struct option long_options[] = {
    {"help",           no_argument,       0, 'h'},
    {"version",        no_argument,       0, 'v'},
    {"visualize",      optional_argument, 0, 'V'},
    {"max-time",       required_argument, 0, 't'},
    {"max-bad-areas",  required_argument, 0, 'b'},
    {"intro-length",   required_argument, 0, 'i'},
    {"outro-length",   required_argument, 0, 'o'},
    {"pop-limit",      required_argument, 0, 'p'},
    {"drop-limit",     required_argument, 0, 'd'},
    {"dupe-limit",     required_argument, 0, 'u'},
    {"pop-drop-dist",  required_argument, 0,  0 },
    {"dupe-dist",      required_argument, 0,  0 },
    {"min-dupes",      required_argument, 0,  0 },
    {"window-size",    required_argument, 0, 'w'},
    {"image-filename", required_argument, 0,  0 },
    {0,                0,                 0,  0 }
};

static int parse_size(const char *str, size_t *size)
{
    char *endptr = NULL;
    unsigned long long value = strtoull(str, &endptr, 10);

    if (*endptr != '\0' || endptr == str) {
        return EINVAL;
    }
    else if (value > SIZE_MAX) {
        return ERANGE;
    }

    *size = (size_t)value;

    return 0;
}

static void usage (int argc, char *argv[])
{
    printf(
        "Usage: %s [OPTIONS] [WAVE-FILE]...\n"
        "'ripcheck' runs a variety of tests on a PCM WAV file, to see if there are potential\n"
        "mistakes that occurred in converting a CD to a WAV file.\n"
        "\n"
        "For more information visit:\n"
        "  http://blog.magnatune.com/2013/09/ripcheck-detect-defects-in-cd-rips.html\n"
        "  https://github.com/panzi/ripcheck\n"
        "\n"
        "Options:\n"
        "\n"
        "  -h, --help                  print this help message\n"
        "  -v, --version               print version information\n"
#ifdef WITH_VISUALIZE
        "  -V, --visualize[=PARAMS]    print wave forms around found problems to PNG images\n"
        "                              PARAMS is a comma separated list of key-value pairs that\n"
        "                              define the size and color of the generated images.\n"
        "\n"
        "                              samp-width=PIXELS      width of a sample (default: 20)\n"
        "                              samp-height=PIXELS     height of a samle above the zero line\n"
        "                                                     (default: 50)\n"
        "                              bg-color=COLOR         background color (default: #FFFFFF)\n"
        "                              wave-color=COLOR       color of the wave form (default: #2084FF)\n"
        "                              zero-color=COLOR       color of the zero line (default: #7F7F7F)\n"
        "                              error-color=COLOR      color of the error sample (default: #FF2020)\n"
        "                              error-bg-color=COLOR   background color of the error sample\n"
        "                                                     (default: #FFC440)\n"
        "\n"
        "                              COLOR may be a HTML like hexadecimal color string (e.g. #FFFFFF)\n"
        "                              or one of the 16 defined HTML color names (e.g. white).\n"
        "\n"
        "      --image-filename=PATTERN\n"
        "                              TODO\n"
        "\n"
#endif
        "  -t, --max-time=TIME         stop analyzing at TIME\n"
        "  -b, --max-bad-areas=COUNT   stop analyzing after COUNT problems found\n"
        "  -i, --intro-length=TIME     start analyzing at TIME (default: 5 sec)\n"
        "  -o, --outro-length=TIME     stop analyzing at TIME before end (default: 5 sec)\n"
        "  -p, --pop-limit=VOLUME      set the minimum volume of a pop to VOLUME (default: 33.333 %%)\n"
        "  -d, --drop-limit=VOLUME     set the minimum volume of samples around a drop to VOLUME\n"
        "                              (default: 66.666 %%)\n"
        "      --pop-drop-dist=TIME    ignore drops before TIME after a pop (default: 8 sampels)\n"
        "  -u, --dupe-limit=VOLUME     ignore dupes more silent than VOLUME (default: 0.033 %%)\n"
        "      --min-dupes=COUNT       set the minimum repetiton of the same sample that is\n"
        "                              recognized as a dupe to COUNT (default: 400)\n"
        "      --dupe-dist=TIME        ignore dupes that follow closer than TIME to another dupe\n"
        "                              (default: 1 sample)\n"
        "  -w, --window-size=COUNT     print COUNT samples when a problem is found (minimum: 7)\n"
        "                              Even if COUNT is bigger ripcheck does not use more than 7\n"
        "                              samples at a time for detecting problems. (default: 7)\n"
        "\n"
        "Units:\n"
        "\n"
        "  TIME\n"
        "    TIME values can be given in samples, seconds or milliseconds.\n"
        "    Examples: 400 samp, 5 sec, 4320 msec\n"
        "\n"
        "    samp, (node) ... samples\n"
        "    sec, s ......... seconds\n"
        "    msec, ms ....... millieseconds\n"
        "\n"
        "  VOLUME\n"
        "    VOLUME values can be given in bit rate dependant values or in percentages.\n"
        "    Examples: 32000, 33.33 %%\n"
        "\n"
        "    (node) ... bit rate dependant absolute volume\n"
        "    %% ........ percentage of maximum possible volume\n"
        "\n"
        "Report bugs to: https://github.com/panzi/ripcheck/issues\n",
        argc > 0 ? argv[0] : "ripcheck");
}

int main (int argc, char *argv[])
{
    // initialize with default values
    ripcheck_time_t max_time      = { SIZE_MAX, RIPCHECK_SAMP };
    ripcheck_time_t intro_length  = { 5, RIPCHECK_SEC };
    ripcheck_time_t outro_length  = { 5, RIPCHECK_SEC };
    ripcheck_time_t pop_drop_dist = { 8, RIPCHECK_SAMP };
    ripcheck_time_t dupe_dist     = { 1, RIPCHECK_SAMP };
    ripcheck_volume_t pop_limit   = { .volume.ratio = 0.33333, .unit = RIPCHECK_RATIO };
    ripcheck_volume_t drop_limit  = { .volume.ratio = 0.66666, .unit = RIPCHECK_RATIO };
    ripcheck_volume_t dupe_limit  = { .volume.ratio = 0.00033, .unit = RIPCHECK_RATIO };
    size_t min_dupes     = 400;
    size_t max_bad_areas = SIZE_MAX;
    size_t window_size   = RIPCHECK_MIN_WINDOW_SIZE;
    struct ripcheck_callbacks callbacks = ripcheck_callbacks_print_text;

#ifdef WITH_VISUALIZE
    struct ripcheck_image_options image_options = {
        .sample_width   =  5,
        .sample_height  = 50,
        .bg_color       = { 255, 255, 255 },
        .wave_color     = {  32, 132, 255 },
        .zero_color     = { 127, 127, 127 },
        .error_color    = { 255,  32,  32 },
        .error_bg_color = { 255, 196,  64 },
        .filename       = "{basename}_sample_{first_error_sample}_channel_{channel}_{errorname}.png"
    };
#endif

    int opt = 0, longindex = 0;
    while ((opt = getopt_long(argc, argv, "hvV,t:b:i:o:p:d:u:w:", long_options, &longindex)) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage(argc, argv);
                return 0;

            case 'v':
                printf("%s\n", RIPCHECK_VERSION);
                return 0;

            case 'V':
#ifdef WITH_VISUALIZE
                if (optarg && ripcheck_parse_image_options(optarg, &image_options) != 0) {
                    fprintf(stderr, "Illegal value for --visualize: %s\n", optarg);
                    return 1;
                }

                callbacks.data          = &image_options;
                callbacks.possible_pop  = ripcheck_image_possible_pop;
                callbacks.possible_drop = ripcheck_image_possible_drop;
                callbacks.dupes         = ripcheck_image_dupes;
                break;

#else
                fprintf(stderr,"Not compiled with support for writing images.\n");
                return 1;
#endif

            case 't':
                if (ripcheck_parse_time(optarg, &max_time) != 0) {
                    fprintf(stderr, "Illegal value for --max-time: %s\n", optarg);
                    return 1;
                }
                break;

            case 'i':
                if (ripcheck_parse_time(optarg, &intro_length) != 0) {
                    fprintf(stderr, "Illegal value for --intro-length: %s\n", optarg);
                    return 1;
                }
                break;

            case 'o':
                if (ripcheck_parse_time(optarg, &outro_length) != 0) {
                    fprintf(stderr, "Illegal value for --outro-length: %s\n", optarg);
                    return 1;
                }
                break;

            case 'p':
                if (ripcheck_parse_volume(optarg, &pop_limit) != 0) {
                    fprintf(stderr, "Illegal value for --pop-limit: %s\n", optarg);
                    return 1;
                }
                break;

            case 'd':
                if (ripcheck_parse_volume(optarg, &drop_limit) != 0) {
                    fprintf(stderr, "Illegal value for --drop-limit: %s\n", optarg);
                    return 1;
                }
                break;

            case 'u':
                if (ripcheck_parse_volume(optarg, &dupe_limit) != 0) {
                    fprintf(stderr, "Illegal value for --dupe-limit: %s\n", optarg);
                    return 1;
                }
                break;

            case 'b':
                if (parse_size(optarg, &max_bad_areas) != 0 || max_bad_areas == 0) {
                    fprintf(stderr, "Illegal value for --max-bad-areas: %s\n", optarg);
                    return 1;
                }
                break;

            case 'w':
                if (parse_size(optarg, &window_size) != 0 || window_size < RIPCHECK_MIN_WINDOW_SIZE) {
                    fprintf(stderr, "Illegal value for --window-size (minimum is %"PRIzu"): %s\n",
                        RIPCHECK_MIN_WINDOW_SIZE, optarg);
                    return 1;
                }
                break;

            case 0:
                switch (longindex) {
                    case 10:
                        if (ripcheck_parse_time(optarg, &pop_drop_dist) != 0) {
                            fprintf(stderr, "Illegal value for --pop-drop-dist: %s\n", optarg);
                            return 1;
                        }
                        break;

                    case 11:
                        if (ripcheck_parse_time(optarg, &dupe_dist) != 0) {
                            fprintf(stderr, "Illegal value for --dupe-dist: %s\n", optarg);
                            return 1;
                        }
                        break;

                    case 12:
                        if (parse_size(optarg, &min_dupes) != 0 || min_dupes <= 1) {
                            fprintf(stderr, "Illegal value for --min-dupes: %s\n", optarg);
                            return 1;
                        }
                        break;

                    case 14:
#ifdef WITH_VISUALIZE
                        // TODO: validate format string now
                        image_options.filename = optarg;
                        break;
#else
                        fprintf(stderr,"Not compiled with support for writing images.\n");
                        return 1;
#endif

                    default:
                        fprintf(stderr, "See --help for usage information.\n");
                        return 255;
                }
                break;

            default:
                fprintf(stderr, "See --help for usage information.\n");
                return 255;
        }
    }

    if (optind >= argc) {
        return ripcheck(stdin, "<stdin>", max_time, intro_length, outro_length, pop_drop_dist,
            dupe_dist, pop_limit, drop_limit, dupe_limit, min_dupes, max_bad_areas, window_size,
            &callbacks) == 0 ? 0 : 1;
    }
    else {
        for (int i = optind; i < argc; ++ i) {
            FILE *f = fopen(argv[i], "rb");

            if (f) {
                int errnum = ripcheck(f, argv[i], max_time, intro_length, outro_length, pop_drop_dist,
                    dupe_dist, pop_limit, drop_limit, dupe_limit, min_dupes, max_bad_areas, window_size,
                    &callbacks);
                fclose(f);

                if (errnum != 0) {
                    return 1;
                }
            }
            else {
                perror(argv[i]);
            }
        }
    }

    return 0;
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
