#include <getopt.h>
#include <errno.h>

#include "ripcheck.h"
#include "print_text.h"

#ifdef WITH_IMAGE
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
    else if (value < 0 || value > SIZE_MAX) {
        return ERANGE;
    }

    *size = (size_t)value;

    return 0;
}

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
        else if (sample_height <= 0 || sample_height > SIZE_MAX) {
            return ERANGE;
        }

        image_options->sample_height = sample_height;
    }
    else if (*endptr != '\0') {
        return EINVAL;
    }

    if (sample_width <= 0 || sample_width > SIZE_MAX) {
        return ERANGE;
    }

    image_options->sample_width = sample_width;

    return 0;
}

static void usage (int argc, char *argv[])
{
    // TODO
    printf("usage: %s [options] <file>...\n\nTODO\n", argc > 0 ? argv[0] : "ripcheck");
}

int main (int argc, char *argv[])
{
    // initialize with default values
	ripcheck_time_t max_time     = { SIZE_MAX, RIPCHECK_SAMP };
	ripcheck_time_t intro_length = { 5, RIPCHECK_SEC };
	ripcheck_time_t outro_length = { 5, RIPCHECK_SEC };
	ripcheck_value_t pop_limit   = { .ratio = 0.33333, .unit = RIPCHECK_RATIO };
	ripcheck_value_t drop_limit  = { .ratio = 0.66666, .unit = RIPCHECK_RATIO };
	ripcheck_value_t dupe_limit  = { .ratio = 0.00033, .unit = RIPCHECK_RATIO };
	size_t min_dupes     = 400;
	size_t max_bad_areas = SIZE_MAX;
    size_t window_size   = RIPCHECK_MIN_WINDOW_SIZE;
    struct ripcheck_callbacks callbacks = ripcheck_callbacks_print_text;

#ifdef WITH_IMAGE
    struct ripcheck_image_options image_options = { 20, 50 };
#endif

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "hvV,t:b:i:o:p:d:u:m:w:", long_options, NULL)) != -1)
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
#ifdef WITH_IMAGE
                if (optarg && parse_image_options(optarg, &image_options) != 0) {
                    fprintf(stderr, "Illegal value for --visualize: %s\n", optarg);
                }

                callbacks.data          = &image_options;
                callbacks.possible_pop  = ripcheck_image_possible_pop;
                callbacks.possible_drop = ripcheck_image_possible_drop;
                callbacks.dupes         = ripcheck_image_dupes;
                break;

#else
                fprintf("Not compiled with support for writing images.\n");
                return 1;
#endif

            case 't':
                if (ripcheck_parse_time(optarg, &max_time) != 0) {
                    fprintf(stderr, "Illegal value for --max-time: %s\n", optarg);
                    return 1;
                }
                break;

            case 'i':
                if (ripcheck_parse_time(optarg, &intro_end) != 0) {
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
        return ripcheck(stdin, "<stdin>", max_time, intro_length, outro_length,
            pop_limit, drop_limit, dupe_limit, min_dupes, max_bad_areas, window_size,
            &callbacks) == 0 ? 0 : 1;
    }
    else {
        for (int i = optind; i < argc; ++ i) {
            FILE *f = fopen(argv[i], "rb");

            if (f) {
                int errnum = ripcheck(f, argv[i], max_time, intro_length, outro_length,
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
