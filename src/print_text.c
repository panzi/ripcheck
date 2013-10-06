#include <stdarg.h>

#include "ripcheck.h"

void ripcheck_print_event(const struct ripcheck_context *context, const char *what, size_t sample, uint16_t channel)
{
    double time = (1000.0L * sample) / context->fmt.sample_rate;
    printf("%s: sample = %"PRIzu" (time = %g ms), channel = %u, values = [",
        what, sample, time, channel);

    for (size_t i = context->window_size; i > 0;) {
        -- i;
        printf("%d", context->window[context->fmt.channels * i + channel]);
        if (i > 0) printf(", ");
    }

    printf("]\n");
}

void ripcheck_text_begin(
    void *data,
	const struct ripcheck_context *context)
{
    fprintf(stderr, "File: %s\n", context->filename);
    fprintf(stderr, "[RIFF] (%u bytes)\n", context->riff_header.size);
    fprintf(stderr, "[WAVEfmt ] (%u bytes)\n", context->riff_header.chunk.size);
    fprintf(stderr, "  Audio format = %u (1 = PCM)\n", context->fmt.audio_format);
    fprintf(stderr, "  Number of channels = %u (1 = mono, 2 = stereo)\n", context->fmt.channels);
    fprintf(stderr, "  Sample rate = %uHz\n", context->fmt.sample_rate);
    fprintf(stderr, "  Bytes / second = %u\n", context->fmt.byte_rate);
    fprintf(stderr, "  Block alignment = %u\n", context->fmt.block_align);
    fprintf(stderr, "  Bits / sample = %u\n", context->fmt.bits_per_sample);
}

void ripcheck_text_sample_data(
    void *data,
	const struct ripcheck_context *context,
    uint32_t data_size)
{
    double duration = (double)data_size / context->fmt.byte_rate;
    fprintf(stderr, "  Data size = %u bytes\n", data_size);
    fprintf(stderr, "  Duration = %g sec\n", duration);
}

void ripcheck_text_possible_pop(
    void        *data,
	const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel)
{
    ripcheck_print_event(context, "pop", sample, channel);
}

void ripcheck_text_possible_drop(
    void        *data,
	const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel)
{
    ripcheck_print_event(context, "drop", sample, channel);
}

void ripcheck_text_dupes(
    void        *data,
	const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel)
{
    double time = (1000.0L * sample) / context->fmt.sample_rate;
    printf("dupes: sample = %"PRIzu" (time = %g ms), channel = %u, dupes = %"PRIzu", values = [",
        sample, time, channel, context->dupecounts[channel]);

    for (size_t i = context->window_size; i > 0;) {
        -- i;
        printf("%d", context->window[context->fmt.channels * i + channel]);
        if (i > 0) printf(", ");
    }

    printf("]\n");
}

void ripcheck_text_complete(
    void *data,
	const struct ripcheck_context *context)
{
    if (context->bad_areas == 0) {
        printf("done: all ok\n");
    }
    else {
        printf("done: %"PRIzu" bad areas found\n", context->bad_areas);
    }
}

void ripcheck_text_error(
    void *data,
	const struct ripcheck_context *context,
    int errnum,
    const char *fmt, ...)
{
    va_list ap;
	fprintf(stderr, "error: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
	fprintf(stderr, "\n");
}

void ripcheck_text_warning(
    void *data,
	const struct ripcheck_context *context,
    const char *fmt, ...)
{
    va_list ap;
	fprintf(stderr, "warning: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
	fprintf(stderr, "\n");
}

struct ripcheck_callbacks ripcheck_callbacks_print_text = {
	NULL,
    ripcheck_text_begin,
    ripcheck_text_sample_data,
    ripcheck_text_possible_pop,
    ripcheck_text_possible_drop,
    ripcheck_text_dupes,
    ripcheck_text_complete,
    ripcheck_text_error,
    ripcheck_text_warning
};

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
