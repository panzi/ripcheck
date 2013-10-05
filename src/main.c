#include <getopt.h>
#include <errno.h>

#include "ripcheckc.h"

const struct option long_options[] = {
    {"help",          no_argument,       0, 'h'},
    {"version",       no_argument,       0, 'v'},
    {"visualize",     no_argument,       0, 'V'},
    {"max-time",      required_argument, 0, 't'},
    {"max-bad-areas", required_argument, 0, 'b'},
    {"intro-end",     required_argument, 0, 'i'},
    {"outro-start",   required_argument, 0, 'o'},
    {"pop-limit",     required_argument, 0, 'p'},
    {"drop-limit",    required_argument, 0, 'd'},
    {"dupe-limit",    required_argument, 0, 'u'},
    {"min-dupes",     required_argument, 0, 'm'},
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

static void usage (int argc, char *argv[])
{
    // TODO
    printf("usage: %s [options] <file>...\n", argc > 0 ? argv[0] : "ripcheck");
}

int main (int argc, char *argv[])
{
    // initialize with default values
	ripcheck_time_t max_time    = { SIZE_MAX, RIPCHECK_SAMP };
	ripcheck_time_t intro_end   = { 5, RIPCHECK_SEC };
	ripcheck_time_t outro_start = { 5, RIPCHECK_SEC };
	ripcheck_value_t pop_limit  = { .ratio = 0.33333, .unit = RIPCHECK_RATIO };
	ripcheck_value_t drop_limit = { .ratio = 0.66666, .unit = RIPCHECK_RATIO };
	ripcheck_value_t dupe_limit = { .ratio = 0.00033, .unit = RIPCHECK_RATIO };
	size_t min_dupes     = 400;
	size_t max_bad_areas = SIZE_MAX;
    struct ripcheck_callbacks callbacks = ripcheck_callbacks_print_text;

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "hvVt:b:i:o:p:d:u:m:", long_options, NULL)) != -1)
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
                // TODO
                printf("TODO: --visualize\n");
                return 1;

            case 't':
                if (ripcheck_parse_time(optarg, &max_time) != 0) {
                    fprintf(stderr, "Illegal value for --max-time: %s\n", optarg);
                    return 1;
                }
                break;

            case 'i':
                if (ripcheck_parse_time(optarg, &intro_end) != 0) {
                    fprintf(stderr, "Illegal value for --intro-end: %s\n", optarg);
                    return 1;
                }
                break;

            case 'o':
                if (ripcheck_parse_time(optarg, &outro_start) != 0) {
                    fprintf(stderr, "Illegal value for --outro-start: %s\n", optarg);
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

            default:
                fprintf(stderr, "See --help for usage information.\n");
                return 255;
        }
    }

    if (optind >= argc) {
        return ripcheck(stdin, "<stdin>", max_time, intro_end, outro_start,
            pop_limit, drop_limit, dupe_limit, min_dupes, max_bad_areas, &callbacks) == 0 ? 0 : 1;
    }
    else {
        for (int i = optind; i < argc; ++ i) {
            FILE *f = fopen(argv[i], "rb");

            if (f) {
                int errnum = ripcheck(f, argv[i], max_time, intro_end, outro_start,
                    pop_limit, drop_limit, dupe_limit, min_dupes, max_bad_areas, &callbacks);
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