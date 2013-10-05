#include <stdarg.h>

#include "ripcheck.h"

void ripcheck_print_event(const struct ripcheck_context *context, const char *what, size_t sample, uint16_t channel)
{
    size_t time = sample * context->fmt.sample_rate;
    printf("%s: sample = %"PRIzu" (%"PRIzu" ms), channel = %u, values = [", what, sample, time, channel);
    for (size_t i = 0; i < context->window_size; ++ i) {
        if (i > 0) printf(", ");
        printf("%d", context->window[context->fmt.channels * i + channel]);
    }
    printf("]\n");
}

static void ripcheck_txt_begin(
    void *data,
	const struct ripcheck_context *context)
{
    fprintf(stderr, "File: %s\n", context->filename);
    fprintf(stderr, "[RIFF] (%u bytes)\n", context->riff_header.size);
    fprintf(stderr, "[WAVEfmt ] (%u bytes)\n", context->riff_header.chunk.size);
    fprintf(stderr, "  Data type = %u (1 = PCM)\n", context->fmt.audio_format);
    fprintf(stderr, "  Number of channels = %u (1 = mono, 2 = stereo)\n", context->fmt.channels);
    fprintf(stderr, "  Sampling rate = %uHz\n", context->fmt.sample_rate);
    fprintf(stderr, "  Bytes / second = %u\n", context->fmt.byte_rate);
    fprintf(stderr, "  Block alignment = %u\n", context->fmt.block_align);
    fprintf(stderr, "  Bits / sample = %u\n", context->fmt.bits_per_sample);
}

static void ripcheck_txt_sample_data(
    void *data,
	const struct ripcheck_context *context,
    uint32_t data_size)
{
    size_t duration = (data_size / context->fmt.block_align) * context->fmt.sample_rate;
    fprintf(stderr, "  Duration = %"PRIzu" milliseconds\n", duration);
}

static void ripcheck_txt_possible_pop(
    void        *data,
	const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel)
{
    ripcheck_print_event(context, "pop", sample, channel);
}

static void ripcheck_txt_possible_drop(
    void        *data,
	const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel)
{
    ripcheck_print_event(context, "drop", sample, channel);
}

static void ripcheck_txt_dupes(
    void        *data,
	const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel)
{
    ripcheck_print_event(context, "dupes", sample, channel);
}

static void ripcheck_txt_complete(
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

static void ripcheck_txt_error(
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

static void ripcheck_txt_warning(
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
    ripcheck_txt_begin,
    ripcheck_txt_sample_data,
    ripcheck_txt_possible_pop,
    ripcheck_txt_possible_drop,
    ripcheck_txt_dupes,
    ripcheck_txt_complete,
    ripcheck_txt_error,
    ripcheck_txt_warning
};

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
