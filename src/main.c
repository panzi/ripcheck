#include <getopt.h>
#include <errno.h>

#include "ripcheck.h"
#include "print_text.h"

#ifdef WITH_VISUALIZE
#include "print_image.h"
#endif

const struct option long_options[] = {
    {"help",          no_argument,       0, 'h'},
    {"version",       no_argument,       0, 'v'},
    {"visualize",     optional_argument, 0, 'V'},
    {"max-time",      required_argument, 0, 't'},
    {"max-bad-areas", required_argument, 0, 'b'},
    {"intro-length",  required_argument, 0, 'i'},
    {"outro-length",  required_argument, 0, 'o'},
    {"pop-limit",     required_argument, 0, 'p'},
    {"drop-limit",    required_argument, 0, 'd'},
    {"dupe-limit",    required_argument, 0, 'u'},
    {"pop-drop-dist", required_argument, 0, 's'},
    {"min-dupes",     required_argument, 0, 'm'},
    {"window-size",   required_argument, 0, 'w'},
    {0,               0,                 0,  0 }
};

static int parse_size(const char *str, size_t *size)
{
    char *endptr = NULL;
    long long value = strtoll(str, &endptr, 10);

    if (*endptr != '\0' || endptr == str) {
        return EINVAL;
    }
    else if (value < 0 || (unsigned long long)value > SIZE_MAX) {
        return ERANGE;
    }

    *size = (size_t)value;

    return 0;
}

#ifdef WITH_VISUALIZE
static int parse_image_options(const char *str, struct ripcheck_image_options *image_options)
{
    if (*str == '\0') {
        return EINVAL;
    }

    char *endptr = NULL;
    long long sample_width = strtoll(str, &endptr, 10);

    if (*endptr == 'x') {
        long long sample_height = strtoll(str, &endptr, 10);

        if (*endptr != '\0') {
            return EINVAL;
        }
        else if (sample_height <= 0 || (unsigned long long)sample_height > SIZE_MAX) {
            return ERANGE;
        }

        image_options->sample_height = sample_height;
    }
    else if (*endptr != '\0') {
        return EINVAL;
    }

    if (sample_width <= 0 || (unsigned long long)sample_width > SIZE_MAX) {
        return ERANGE;
    }

    image_options->sample_width = sample_width;

    return 0;
}
#endif

static void usage (int argc, char *argv[])
{
    printf(
        "Usage: %s [OPTIONS] [WAVE-FILE]...\n"
        "'ripcheck' runs a variety of tests on a PCM WAV file, to see if there are "
        "potential mistakes that occurred in converting a CD to a WAV file.\n"
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
        "  -V, --visualize[=PARAMS]    print wave forms around found problems to PNG files\n"
        "                              TODO: describe PRAMS\n"
#endif
        "  -t, --max-time=TIME         stop analyzing at TIME\n"
        "  -b, --max-bad-areas=COUNT   stop analyzing after COUNT problems found\n"
        "  -i, --intro-length=TIME     start analyzing at TIME (default: 5 sec)\n"
        "  -o, --outro-length=TIME     stop analyzng at TIME before end (default: 5 sec)\n"
        "  -p, --pop-limit=VOLUME      set the minimum volume of a pop to VOLUME (default: 33.333 %%)\n"
        "  -d, --drop-limit=VOLUME     set the minimum volume of samples around a drop to VOLUME\n"
        "                              (default: 66.666 %%)\n"
        "  -s, --pop-drop-dist=TIME    ignore drops before TIME after a pop (default: 8 sampels)\n"
        "  -u, --dupe-limit=VOLUME     ignore dupes more silent than VOLUME (default: 0.033 %%)\n"
        "  -m, --min-dupes=COUNT       set the minimum repetiton of the same sample that is\n"
        "                              recognized as a dupe to COUNT (default: 400)\n"
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
        "    VOLUME values can be given in bit rate dependant values or percentages.\n"
        "    Examples: 32000, 33.33 %%\n"
        "\n"
        "    (node) ... bit rate dependant absolute volume\n"
        "    %% ........ percentage of maximum possible voluem\n"
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
    ripcheck_volume_t pop_limit   = { .volume.ratio = 0.33333, .unit = RIPCHECK_RATIO };
    ripcheck_volume_t drop_limit  = { .volume.ratio = 0.66666, .unit = RIPCHECK_RATIO };
    ripcheck_volume_t dupe_limit  = { .volume.ratio = 0.00033, .unit = RIPCHECK_RATIO };
    size_t min_dupes     = 400;
    size_t max_bad_areas = SIZE_MAX;
    size_t window_size   = RIPCHECK_MIN_WINDOW_SIZE;
    struct ripcheck_callbacks callbacks = ripcheck_callbacks_print_text;

#ifdef WITH_VISUALIZE
    struct ripcheck_image_options image_options = { 20, 50 };
#endif

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "hvV,t:b:i:o:p:d:u:m:w:s:", long_options, NULL)) != -1)
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
                if (optarg && parse_image_options(optarg, &image_options) != 0) {
                    fprintf(stderr, "Illegal value for --visualize: %s\n", optarg);
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
                if (ripcheck_parse_value(optarg, &pop_limit) != 0) {
                    fprintf(stderr, "Illegal value for --pop-limit: %s\n", optarg);
                    return 1;
                }
                break;

            case 'd':
                if (ripcheck_parse_value(optarg, &drop_limit) != 0) {
                    fprintf(stderr, "Illegal value for --drop-limit: %s\n", optarg);
                    return 1;
                }
                break;

            case 'u':
                if (ripcheck_parse_value(optarg, &dupe_limit) != 0) {
                    fprintf(stderr, "Illegal value for --dupe-limit: %s\n", optarg);
                    return 1;
                }
                break;

            case 's':
                if (ripcheck_parse_time(optarg, &pop_drop_dist) != 0) {
                    fprintf(stderr, "Illegal value for --pop-drop-dist: %s\n", optarg);
                    return 1;
                }
                break;

            case 'm':
                if (parse_size(optarg, &min_dupes) != 0 || min_dupes <= 1) {
                    fprintf(stderr, "Illegal value for --min-dupes: %s\n", optarg);
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

            default:
                fprintf(stderr, "See --help for usage information.\n");
                return 255;
        }
    }

    if (optind >= argc) {
        return ripcheck(stdin, "<stdin>", max_time, intro_length, outro_length, pop_drop_dist,
            pop_limit, drop_limit, dupe_limit, min_dupes, max_bad_areas, window_size,
            &callbacks) == 0 ? 0 : 1;
    }
    else {
        for (int i = optind; i < argc; ++ i) {
            FILE *f = fopen(argv[i], "rb");

            if (f) {
                int errnum = ripcheck(f, argv[i], max_time, intro_length, outro_length, pop_drop_dist,
                    pop_limit, drop_limit, dupe_limit, min_dupes, max_bad_areas, window_size,
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
