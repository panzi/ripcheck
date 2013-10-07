#ifndef RIPCHECKC_H__
#define RIPCHECKC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)
#    ifdef _WIN64
#        define PRIzu PRIu64
#        define PRIzx PRIx64
#    else
#        define PRIzu PRIu32
#        define PRIzx PRIx32
#    endif
#else
#    define PRIzu "zu"
#    define PRIzx "zx"
#endif

#ifndef __GNUC__
#    define __attribute__(X)
#endif

#define _RIPCHECK_STR(V) #V
#define RIPCHECK_STR(V) _RIPCHECK_STR(V)

#define RIPCHECK_VERSION_MAJOR 1
#define RIPCHECK_VERSION_MINOR 0
#define RIPCHECK_VERSION_PATCH 0

#define RIPCHECK_VERSION \
    RIPCHECK_STR(RIPCHECK_VERSION_MAJOR) "." \
    RIPCHECK_STR(RIPCHECK_VERSION_MINOR) "." \
    RIPCHECK_STR(RIPCHECK_VERSION_PATCH)

#define RIPCHECK_MIN_WINDOW_SIZE (size_t)7

// a RIFF WAVE struct should be properly aligned anyway, but just to be sure use pragma pack
#pragma pack(push, 1)
struct riff_chunk_header {
    uint8_t  id[4];
    uint32_t size;
};

struct riff_header {
    uint8_t  id[4];
    uint32_t size;
    uint8_t  format[4];
    struct riff_chunk_header chunk;
};

struct wave_fmt {
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};
#pragma pack(pop)

enum ripcheck_value_unit {
    RIPCHECK_RATIO,
    RIPCHECK_ABSOLUTE
};

typedef struct ripcheck_value {
    union {
        double ratio;
        int    absolute;
    } volume;
    enum ripcheck_value_unit unit;
} ripcheck_volume_t;

enum ripcheck_time_unit {
    RIPCHECK_SEC,
    RIPCHECK_MSEC,
    RIPCHECK_SAMP
};

typedef struct ripcheck_time {
    size_t time;
    enum ripcheck_time_unit unit;
} ripcheck_time_t;

struct ripcheck_context {
    const char *filename;
    size_t max_sample;
    size_t intro_length;
    size_t outro_length;
    size_t pop_drop_dist;
    int    pop_limit;
    int    drop_limit;
    int    dupe_limit;
    size_t min_dupes;
    struct riff_header riff_header;
    struct wave_fmt    fmt;
    uint8_t *frame;
    int     *window;
    size_t   window_size;
    size_t  *dupecounts;
    size_t  *poplocs;
    size_t  *badlocs;
    size_t   bad_areas;
    size_t   max_bad_areas;
};

int ripcheck_parse_volume(const char *str, ripcheck_volume_t *volume);
int ripcheck_parse_time(const char *str, ripcheck_time_t *time);

/* Callback Types */
typedef void (*ripcheck_begin_t)(
    void        *data,
    const struct ripcheck_context *context);

typedef void (*ripcheck_sample_data_t)(
    void        *data,
    const struct ripcheck_context *context,
    uint32_t     data_size);

typedef void (*ripcheck_possible_pop_t)(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel);

typedef void (*ripcheck_possible_drop_t)(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel);

typedef void (*ripcheck_dupes_t)(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel);

typedef void (*ripcheck_complete_t)(
    void        *data,
    const struct ripcheck_context *context);

typedef void (*ripcheck_error_t)(
    void        *data,
    const struct ripcheck_context *context,
    int          errnum,
    const char  *fmt,
    ...) __attribute__ ((format (printf, 4, 5)));

typedef void (*ripcheck_warning_t)(
    void        *data,
    const struct ripcheck_context *context,
    const char  *fmt,
    ...) __attribute__ ((format (printf, 3, 4)));

struct ripcheck_callbacks {
    // data that gets passed to the callback functions
    // currently unused
    void *data;

    ripcheck_begin_t         begin;
    ripcheck_sample_data_t   sample_data;
    ripcheck_possible_pop_t  possible_pop;
    ripcheck_possible_drop_t possible_drop;
    ripcheck_dupes_t         dupes;
    ripcheck_complete_t      complete;
    ripcheck_error_t         error;
    ripcheck_warning_t       warning;
};

int ripcheck(
    FILE *f,
    const char *filename,
    ripcheck_time_t max_time,
    ripcheck_time_t intro_length,
    ripcheck_time_t outro_length,
    ripcheck_time_t pop_drop_dist,
    ripcheck_volume_t pop_limit,
    ripcheck_volume_t drop_limit,
    ripcheck_volume_t dupe_limit,
    size_t window_size,
    size_t min_dupes,
    size_t max_bad_areas,
    struct ripcheck_callbacks *callbacks);

#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
