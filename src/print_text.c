#include <stdarg.h>

#include "ripcheck.h"

void ripcheck_print_event(
    const struct ripcheck_context *context, const char *what, uint16_t channel,
    size_t last_window_sample, size_t first_error_sample, size_t last_error_sample)
{
    double time = (1000.0L * first_error_sample) / context->fmt.sample_rate;
    if (first_error_sample == last_error_sample) {
        printf("%s: sample = %"PRIzu" (time = %g ms)", what, first_error_sample, time);
    }
    else {
        double end_time = (1000.0L * last_error_sample) / context->fmt.sample_rate;
        printf("%s: samples = %"PRIzu" ... %"PRIzu" (%"PRIzu" samples, time = %g ms ... %g ms)",
            what, first_error_sample, last_error_sample, last_error_sample - first_error_sample + 1,
            time, end_time);
    }
    printf(", channel = %u, samples[%"PRIzu" ... %"PRIzu"] = {", channel,
        last_window_sample - context->window_size + 1, last_window_sample);

    for (size_t i = context->window_size; i > 0;) {
        -- i;
        printf("%d", context->window[context->fmt.channels * i + channel]);
        if (i > 0) printf(", ");
    }

    printf("}\n");
}

void ripcheck_text_begin(
    void *data,
	const struct ripcheck_context *context)
{
    (void)data;
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
    (void)data;
    double duration = (double)data_size / context->fmt.byte_rate;
    fprintf(stderr, "  Data size = %u bytes\n", data_size);
    fprintf(stderr, "  Duration = %g sec\n", duration);
}

void ripcheck_text_possible_pop(
    void        *data,
    const struct ripcheck_context *context,
    uint16_t     channel,
    size_t       last_window_sample)
{
    (void)data;
    ripcheck_print_event(context, "pop", channel, last_window_sample,
        context->poplocs[channel],  context->poplocs[channel]);
}

void ripcheck_text_possible_drop(
    void        *data,
    const struct ripcheck_context *context,
    uint16_t     channel,
    size_t       last_window_sample,
    size_t       droped_sample)
{
    (void)data;
    ripcheck_print_event(context, "drop", channel, last_window_sample,
    droped_sample, droped_sample);
}

void ripcheck_text_dupes(
    void        *data,
    const struct ripcheck_context *context,
    uint16_t     channel,
    size_t       last_window_sample)
{
    (void)data;
    ripcheck_print_event(context, "dupes", channel, last_window_sample,
    context->dupelocs[channel], context->dupelocs[channel] + context->dupecounts[channel] - 1);
}

void ripcheck_text_complete(
    void *data,
	const struct ripcheck_context *context)
{
    (void)data;
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
    (void)data;
    (void)context;
    (void)errnum;

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
    (void)data;
    (void)context;

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
