/*********************************************************************
    ripcheck.c
    
    from:
    http://blog.magnatune.com/2013/09/ripcheck-detect-defects-in-cd-rips.html

    written by John Buckman of Magnatune, Mathias Panzenböck

    Released under GPL v3 license
    http://www.gnu.org/licenses/gpl.html
         
*********************************************************************/

#include <errno.h>
#include <strings.h>
#include <ctype.h>

#include "ripcheck.h"
#include "ripcheck_endian.h"

#define RIFF_HEADER_SIZE 20
#define WAVE_FMT_SIZE    16
#define RIFF_CHUNK_HEADER_SIZE 8

#define PCM 1

static int ripcheck_data(
    FILE *f,
    uint32_t size,
    struct ripcheck_context   *context,
    struct ripcheck_callbacks *callbacks);

static void ripcheck_context_cleanup(struct ripcheck_context *context)
{
    free(context->frame);
    free(context->window);
    free(context->dupecounts);
    free(context->poplocs);
    free(context->dupelocs);
}

static unsigned int to_full_byte(int bits)
{
    int rem = bits % 8;
    return rem == 0 ? bits : bits + (8 - rem);
}

static size_t time_to_samples(const struct ripcheck_context *context, const ripcheck_time_t time)
{
    switch (time.unit)
    {
        case RIPCHECK_SAMP: return time.time;
        case RIPCHECK_SEC:  return context->fmt.sample_rate * time.time;
        case RIPCHECK_MSEC: 
        default:            return context->fmt.sample_rate * time.time / 1000;
    }
}

static int abs_volume(const int max_value, const ripcheck_volume_t volume)
{
    switch (volume.unit)
    {
        case RIPCHECK_RATIO:    return (int)(volume.volume.ratio * max_value);
        case RIPCHECK_ABSOLUTE:
        default:                return volume.volume.absolute;
    }
}

static char *skipws(char *ptr)
{
    while (isspace(*ptr)) ++ ptr;
    return ptr;
}

int ripcheck_parse_volume(const char *str, ripcheck_volume_t *volumeptr)
{
    char *endptr = NULL;
    const size_t len = strlen(str);
    
    if (len == 0) {
        return EINVAL;
    }

    if (str[len - 1] == '%') {
        double ratio = strtod(str, &endptr);

        if (skipws(endptr) != str + len - 1) {
            return EINVAL;
        }
        else if (ratio < 0) {
            return ERANGE;
        }

        volumeptr->unit         = RIPCHECK_RATIO;
        volumeptr->volume.ratio = ratio;
    }
    else {
        unsigned long long absval = strtoull(str, &endptr, 10);

        if (*skipws(endptr) != '\0') {
            return EINVAL;
        }
        else if (absval > SIZE_MAX) {
            return ERANGE;
        }

        volumeptr->unit            = RIPCHECK_ABSOLUTE;
        volumeptr->volume.absolute = absval;
    }

    return 0;
}

int ripcheck_parse_time(const char *str, ripcheck_time_t *timeptr)
{
    char *endptr = NULL;
    unsigned long long time = strtoull(str, &endptr, 10);

    if (endptr == str) {
        return EINVAL;
    }

    if (time > SIZE_MAX) {
        return ERANGE;
    }
    
    endptr = skipws(endptr);
    if (*endptr == '\0' || strcasecmp(endptr, "samp") == 0 ||
        strcasecmp(endptr, "sample") || strcasecmp(endptr, "samples")) {
        timeptr->unit = RIPCHECK_SAMP;
    }
    else if (strcasecmp(endptr, "ms") == 0 || strcasecmp(endptr, "msec") == 0) {
        timeptr->unit = RIPCHECK_MSEC;
    }
    else if (strcasecmp(endptr, "s") == 0 || strcasecmp(endptr, "sec") == 0) {
        timeptr->unit = RIPCHECK_SEC;
    }
    else {
        return EINVAL;
    }

    timeptr->time = time;

    return 0;
}

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
    size_t min_dupes,
    size_t max_bad_areas,
    size_t window_size,
    struct ripcheck_callbacks *callbacks)
{
    struct ripcheck_context context;

    memset(&context, 0, sizeof(context));

    context.filename  = filename;
    context.min_dupes = min_dupes;
    context.max_bad_areas = max_bad_areas;

    // read RIFF file header and chunk id & size of first chunk in one go:
    if (fread(&context.riff_header, RIFF_HEADER_SIZE, 1, f) != 1)
    {
        int errnum = errno;
        callbacks->error(callbacks->data, &context, errnum, "%s", strerror(errnum));
        return errnum;
    }

    // check chunk id of file and first chunk and format of RIFF file
    if (memcmp(context.riff_header.id, "RIFF", 4) != 0) {
        callbacks->error(callbacks->data, &context, EINVAL, "Not a 'RIFF' file: '%c%c%c%c'",
            context.riff_header.id[0],
            context.riff_header.id[1],
            context.riff_header.id[2],
            context.riff_header.id[3]);
        return EINVAL;
    }

    if (memcmp(context.riff_header.format, "WAVE", 4) != 0) {
        callbacks->error(callbacks->data, &context, EINVAL, "Not a 'WAVE' format: '%c%c%c%c'",
            context.riff_header.format[0],
            context.riff_header.format[1],
            context.riff_header.format[2],
            context.riff_header.format[3]);
        return EINVAL;
    }

    if (memcmp(context.riff_header.chunk.id, "fmt ", 4) != 0) {
        callbacks->error(callbacks->data, &context, EINVAL, "WAVE file does not start with a 'fmt ' chunk: '%c%c%c%c'",
            context.riff_header.chunk.id[0],
            context.riff_header.chunk.id[1],
            context.riff_header.chunk.id[2],
            context.riff_header.chunk.id[3]);
        return EINVAL;
    }

    const uint32_t riff_size = le32toh(context.riff_header.size);
    const uint32_t fmt_size  = le32toh(context.riff_header.chunk.size);
    uint32_t pos = fmt_size + 8;

    // sanity check of declared sizes
    if (riff_size < pos || fmt_size < WAVE_FMT_SIZE)
    {
        callbacks->error(callbacks->data, &context, EINVAL,
            "WAVE file has illegal chunk sizes. RIFF size: %u, fmt size: %u",
            riff_size, fmt_size);
        return EINVAL;
    }

    // ignore bytes in fmt chunk after the standard number of bytes
    if (fread(&context.fmt, WAVE_FMT_SIZE, 1, f) != 1 ||
        (fmt_size > WAVE_FMT_SIZE && fseek(f, fmt_size - WAVE_FMT_SIZE, SEEK_CUR) != 0))
    {
        int errnum = errno;
        callbacks->error(callbacks->data, &context, errnum, "%s", strerror(errnum));
        return errnum;
    }

    // convert endian of fmt chunk
    context.fmt.audio_format    = le16toh(context.fmt.audio_format);
    context.fmt.channels        = le16toh(context.fmt.channels);
    context.fmt.sample_rate     = le32toh(context.fmt.sample_rate);
    context.fmt.byte_rate       = le32toh(context.fmt.byte_rate);
    context.fmt.block_align     = le16toh(context.fmt.block_align);
    context.fmt.bits_per_sample = le16toh(context.fmt.bits_per_sample);

    const int max_value = ~(~0 << (context.fmt.bits_per_sample - 1));
    context.pop_limit  = abs_volume(max_value, pop_limit);
    context.drop_limit = abs_volume(max_value, drop_limit);
    context.dupe_limit = abs_volume(max_value, dupe_limit);

    context.max_sample    = time_to_samples(&context, max_time);
    context.intro_length  = time_to_samples(&context, intro_length);
    context.outro_length  = time_to_samples(&context, outro_length);
    context.pop_drop_dist = time_to_samples(&context, pop_drop_dist);

    callbacks->begin(callbacks->data, &context);

    if (context.fmt.audio_format != PCM)
    {
        callbacks->error(callbacks->data, &context, EINVAL, "Not a PCM WAVE file. audio format: %u", context.fmt.audio_format);
        return EINVAL;
    }

    // sanity checks
    if (context.fmt.bits_per_sample == 0)
    {
        callbacks->error(callbacks->data, &context, EINVAL, "Illegal value of bits per sample: %u", context.fmt.bits_per_sample);
        return EINVAL;
    }

    const unsigned int ceil_bits_per_sample = to_full_byte(context.fmt.bits_per_sample * context.fmt.channels);
    if (ceil_bits_per_sample > 8 * context.fmt.block_align)
    {
        callbacks->error(callbacks->data, &context, EINVAL, "WAVE file specifies more bits per sample than fit into one sample. "
            "bits per sample: %u, block alignment: %u", context.fmt.bits_per_sample, context.fmt.block_align);
        return EINVAL;
    }
    
    if (ceil_bits_per_sample > 8 * sizeof(int) * context.fmt.channels)
    {
        callbacks->error(callbacks->data, &context, EINVAL, "Too many bits per sample: %u", context.fmt.bits_per_sample);
        return EINVAL;
    }

    // allocate buffers
    context.frame = malloc(context.fmt.block_align);

    if (!context.frame)
    {
        int errnum = errno;
        callbacks->error(callbacks->data, &context, errnum, "%s", strerror(errnum));
        return errnum;
    }

    context.window_size = window_size < RIPCHECK_MIN_WINDOW_SIZE ? RIPCHECK_MIN_WINDOW_SIZE : window_size;
    context.window = malloc(sizeof(int) * context.fmt.channels * context.window_size);

    if (!context.window)
    {
        int errnum = errno;
        ripcheck_context_cleanup(&context);
        callbacks->error(callbacks->data, &context, errnum, "%s", strerror(errnum));
        return errnum;
    }

    context.dupecounts = malloc(sizeof(size_t) * context.fmt.channels);

    if (!context.dupecounts)
    {
        int errnum = errno;
        ripcheck_context_cleanup(&context);
        callbacks->error(callbacks->data, &context, errnum, "%s", strerror(errnum));
        return errnum;
    }

    context.poplocs = malloc(sizeof(size_t) * context.fmt.channels);

    if (!context.poplocs)
    {
        int errnum = errno;
        ripcheck_context_cleanup(&context);
        callbacks->error(callbacks->data, &context, errnum, "%s", strerror(errnum));
        return errnum;
    }

    context.dupelocs = malloc(sizeof(size_t) * context.fmt.channels);

    if (!context.dupelocs)
    {
        int errnum = errno;
        ripcheck_context_cleanup(&context);
        callbacks->error(callbacks->data, &context, errnum, "%s", strerror(errnum));
        return errnum;
    }

    // read blocks
    while (pos < riff_size)
    {
        struct riff_chunk_header chunk_header;

        if (fread(&chunk_header, RIFF_CHUNK_HEADER_SIZE, 1, f) != 1)
        {
            int errnum = errno;
            ripcheck_context_cleanup(&context);
            callbacks->error(callbacks->data, &context, errnum, "%s", strerror(errnum));
            return errnum;
        }

        uint32_t chunk_size = le32toh(chunk_header.size);

        // TODO: support wave list and silent chunks?
        // http://www.sonicspot.com/guide/wavefiles.html#wavl

        // process data chunk
        if (memcmp(chunk_header.id, "data", 4) == 0)
        {
            int errnum = ripcheck_data(f, chunk_size, &context, callbacks);
            if (errnum != 0)
            {
                ripcheck_context_cleanup(&context);
                return errnum;
            }

            // there may be only one data chunk in a wave file, so stop here
            break;
        }
        // ignore any other chunk
        else if (fseek(f, chunk_size, SEEK_CUR) != 0)
        {
            int errnum = errno;
            ripcheck_context_cleanup(&context);
            callbacks->error(callbacks->data, &context, errnum, "%s", strerror(errnum));
            return errnum;
        }

        pos += RIFF_CHUNK_HEADER_SIZE + chunk_size;
    }

    ripcheck_context_cleanup(&context);
    callbacks->complete(callbacks->data, &context);

    return 0;
}

int ripcheck_data(
    FILE *f,
    uint32_t size,
    struct ripcheck_context   *context,
    struct ripcheck_callbacks *callbacks)
{
    uint8_t *frame      = context->frame;
    int     *window     = context->window;
    size_t  *dupecounts = context->dupecounts;
    size_t  *poplocs    = context->poplocs;
    size_t  *dupelocs   = context->dupelocs;

    const uint16_t channels        = context->fmt.channels;
    const uint16_t block_align     = context->fmt.block_align;
    const uint16_t bits_per_sample = context->fmt.bits_per_sample;

    const size_t   max_bad_areas = context->max_bad_areas;
    const size_t   blocks     = size / block_align;
    const size_t   max_sample = context->max_sample < blocks ? context->max_sample : blocks;
    const unsigned int ceil_bits_per_sample = to_full_byte(bits_per_sample);
    const unsigned int bytes_per_sample     = ceil_bits_per_sample / 8;
    const unsigned int shift                = ceil_bits_per_sample - bits_per_sample;

    // mid is mid-point for unsinged values and bitmask of sign for singed values
    const int mid  = 1 << (bits_per_sample - 1);
    const int mask = ~0 << bits_per_sample;

    const int pop_limit  = context->pop_limit;
    const int drop_limit = context->drop_limit;
    const int dupe_limit = context->dupe_limit;

    const size_t intro_end_sample   = blocks > context->intro_length ? context->intro_length          : blocks;
    const size_t outro_start_sample = blocks > context->outro_length ? blocks - context->outro_length : 0;
    const size_t pop_drop_dist      = context->pop_drop_dist;
    const size_t min_dupes          = context->min_dupes;
    const size_t window_size        = context->window_size;
    const size_t window_shift       = (window_size - 1) * channels * sizeof(int);

    memset(window,     0, sizeof(int)    * channels * window_size);
    memset(dupecounts, 0, sizeof(size_t) * channels);
    memset(poplocs,    0, sizeof(size_t) * channels);
    memset(dupelocs,   0, sizeof(size_t) * channels);

    callbacks->sample_data(callbacks->data, context, size);

    if (blocks * block_align < size)
    {
        // huh, the size of the data chunk in bytes is not a multiple of the blocks
        callbacks->warning(callbacks->data, context,
            "The size of the 'data' chunk (%u) is not a multiple of the block alignment (%u).",
            size, block_align);
    }

    for (size_t sample = 0; sample < max_sample; ++ sample)
    {
        if (fread(frame, block_align, 1, f) != 1)
        {
            int errnum = errno;
            callbacks->error(callbacks->data, context, errnum, "%s", strerror(errnum));
            return errnum;
        }

        // decode samples into first row of window
        for (size_t channel = 0; channel < channels; ++ channel)
        {
            // http://www.neurophys.wisc.edu/auditory/riff-format.txt
            // http://home.roadrunner.com/~jgglatt/tech/wave.htm#POINTS
            //
            // 1 to 8 bits are unsigned
            // 9 and more bits are signed
            int x0 = 0;

            // I guess that this *might* be a performance drain:
            for (size_t byte = 0; byte < bytes_per_sample; ++ byte)
            {
                x0 = (frame[channel + byte] << (byte * 8)) | x0;
            }

            // shift away padding
            x0 >>= shift;

            if (bits_per_sample > 8)
            {
                if (x0 & mid) { // negative
                    window[channel] = x0 = x0 | mask;
                }
                else { // positive
                    window[channel] = x0;
                }
            }
            else
            {
                window[channel] = x0 = x0 - mid;
            }

            // analyze audio per channel
            int x1 = window[channels * 1 + channel];
            int x2 = window[channels * 2 + channel];
            int x3 = window[channels * 3 + channel];
            int x4 = window[channels * 4 + channel];
            int x5 = window[channels * 5 + channel];
            int x6 = window[channels * 6 + channel];

            // look for a pop
            // (x2 ... x6) == 0, abs(x1) > pop_limit
            size_t poploc = sample - 2;
            if (x6 == 0 && x5 == 0 && x4 == 0 && x3 == 0 && (x2 > pop_limit || x2 < -pop_limit) &&
                sample > 4 && poploc < outro_start_sample)
            {
                ++ context->bad_areas;
                poplocs[channel] = poploc;
                callbacks->possible_pop(callbacks->data, context, channel, sample);
                if (context->bad_areas >= max_bad_areas) break;
            }
            else
            {
                poploc = poplocs[channel];
            }

            // look for a dropped sample, but not closer than 8 samples to the previous pop
            // x2 > drop_limit, x1 == 0, x0 > drop_limit
            // x2 < drop_limit, x1 == 0, x0 < drop_limit
            size_t droploc = sample - 1;
            if (x1 == 0 &&
                ((x2 > drop_limit && x0 > drop_limit) || (x2 < -drop_limit && x0 < -drop_limit)) &&
                droploc > poploc + pop_drop_dist &&
                droploc > intro_end_sample &&
                droploc < outro_start_sample)
            {
                ++ context->bad_areas;
//                poplocs[channel] = droploc;
                callbacks->possible_drop(callbacks->data, context, channel, sample, droploc);
                if (context->bad_areas >= max_bad_areas) break;
            }

            // look for duplicates
            if (x0 == x1) {
                ++ dupecounts[channel];
            }
            else {
                size_t dupeloc = sample - dupecounts[channel];
                if ((x1 <= -dupe_limit || x1 >= dupe_limit) &&
                    dupecounts[channel] >= min_dupes &&
                    dupeloc < outro_start_sample &&
                    dupeloc > dupelocs[channel] + intro_end_sample)
                {
                    ++ context->bad_areas;
                    dupelocs[channel] = dupeloc;
                    callbacks->dupes(callbacks->data, context, channel, sample);
                    if (context->bad_areas >= max_bad_areas) break;
                }
                dupecounts[channel] = 0;
            }
        }

        // shift the window
        // TODO: make window a ring buffer to increase speed for big window sizes?
        memmove(window + channels, window, window_shift);
    }

    return 0;
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
