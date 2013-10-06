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
#define WINDOW_SIZE 7

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

static int get_value(const int max_value, const ripcheck_value_t value)
{
    switch (value.unit)
    {
        case RIPCHECK_RATIO:    return (int)(value.ratio * max_value);
        case RIPCHECK_ABSOLUTE:
        default:                return value.absolute;
    }
}

static char *skipws(char *ptr)
{
    while (isspace(*ptr)) ++ ptr;
    return ptr;
}

int ripcheck_parse_value(const char *str, ripcheck_value_t *valueptr)
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

        valueptr->unit  = RIPCHECK_RATIO;
        valueptr->ratio = ratio;
    }
    else {
        long long absval = strtoll(str, &endptr, 10);

        if (*skipws(endptr) != '\0') {
            return EINVAL;
        }
        else if (absval < 0 || absval > SIZE_MAX) {
            return ERANGE;
        }

        valueptr->unit     = RIPCHECK_ABSOLUTE;
        valueptr->absolute = absval;
    }

    return 0;
}

int ripcheck_parse_time(const char *str, ripcheck_time_t *timeptr)
{
    char *endptr = NULL;
    long long time = strtoll(str, &endptr, 10);

    if (endptr == str) {
        return EINVAL;
    }

    if (time < 0 || time > SIZE_MAX) {
        return ERANGE;
    }
    
    endptr = skipws(endptr);
    if (*endptr == '\0' || strcasecmp(endptr, "samp") == 0) {
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
	ripcheck_time_t intro_end,
	ripcheck_time_t outro_start,
	ripcheck_value_t pop_limit,
	ripcheck_value_t drop_limit,
	ripcheck_value_t dupe_limit,
	size_t min_dupes,
	size_t max_bad_areas,
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
    context.pop_limit  = get_value(max_value, pop_limit);
    context.drop_limit = get_value(max_value, drop_limit);
    context.dupe_limit = get_value(max_value, dupe_limit);

    context.max_sample         = time_to_samples(&context, max_time);
    context.intro_end_sample   = time_to_samples(&context, intro_end);
    context.outro_start_sample = time_to_samples(&context, outro_start);

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

    context.window_size = WINDOW_SIZE;
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

    context.badlocs = malloc(sizeof(size_t) * context.fmt.channels);

    if (!context.badlocs)
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
    size_t  *badlocs    = context->badlocs;
    size_t   poploc     = 0;

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

    const size_t intro_end_sample   = blocks > context->intro_end_sample   ? context->intro_end_sample            : blocks;
    const size_t outro_start_sample = blocks > context->outro_start_sample ? blocks - context->outro_start_sample : 0;
    const size_t min_dupes          = context->min_dupes;

    memset(window,     0, sizeof(int)    * channels * context->window_size);
    memset(dupecounts, 0, sizeof(size_t) * channels);
    memset(poplocs,    0, sizeof(size_t) * channels);
    memset(badlocs,    0, sizeof(size_t) * channels);

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
            unsigned int value = 0;

            // I guess that this *might* be a performance drain:
            for (size_t byte = 0; byte < bytes_per_sample; ++ byte)
            {
                value = (frame[channel + byte] << (byte * 8)) | value;
            }

            // shift away padding
            value >>= shift;

            if (bits_per_sample > 8)
            {
                if (value & mid) { // negative
                    window[channel] = (int)(value | mask);
                }
                else { // positive
                    window[channel] = (int)value;
                }
            }
            else
            {
                window[channel] = (int)value - mid;
            }

            // analyze audio per channel

            // look for a pop
            // (x2 ... x6) == 0, abs(x1) > pop_limit
            int x1 = window[channels * 1 + channel];
            int x2 = window[channels * 2 + channel];
            int x3 = window[channels * 3 + channel];
            int x4 = window[channels * 4 + channel];
            int x5 = window[channels * 5 + channel];
            int x6 = window[channels * 6 + channel];

            if (x6 == 0 && x5 == 0 && x4 == 0 && x3 == 0 && (x2 > pop_limit || x2 < -pop_limit) &&
                sample > intro_end_sample &&
                sample < outro_start_sample)
            {
                ++ context->bad_areas;
                poploc = poplocs[channel] = sample;
                callbacks->possible_pop(callbacks->data, context, sample, channel);
                if (context->bad_areas >= max_bad_areas) break;
            }
            else
            {
                poploc = poplocs[channel];
            }

            // look for a dropped sample, but not closer than 8 samples to the previous pop
            // x2 > drop_limit, x1 == 0, x0 > drop_limit
            // x2 < drop_limit, x1 == 0, x0 < drop_limit
            if (x1 == 0 && ((x2 > drop_limit && value > drop_limit) || (x2 < -drop_limit && value < -drop_limit)) &&
                sample > poploc + 8 &&
                sample > intro_end_sample &&
                sample < outro_start_sample)
            {
                ++ context->bad_areas;
                poplocs[channel] = sample;
                callbacks->possible_drop(callbacks->data, context, sample, channel);
                if (context->bad_areas >= max_bad_areas) break;
            }

            // look for duplicates
            if (value == x1) {
                ++ dupecounts[channel];
            }
            else {
                if (value != 0 &&
                    dupecounts[channel] >= min_dupes &&
                    sample < outro_start_sample &&
                    sample > badlocs[channel] + intro_end_sample)
                {
                    if (value <= -dupe_limit || value >= dupe_limit) {
                        ++ context->bad_areas;
                        badlocs[channel] = sample;
                        callbacks->dupes(callbacks->data, context, sample, channel);
                        if (context->bad_areas >= max_bad_areas) break;
                    }
                }
                dupecounts[channel] = 0;
            }
        }

        // shift the window
        memmove(window + channels, window, (WINDOW_SIZE - 1) * channels * sizeof(int));
    }

    return 0;
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
