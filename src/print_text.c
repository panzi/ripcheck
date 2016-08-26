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

#include <stdarg.h>

#include "ripcheck.h"

void ripcheck_print_event(
    const struct ripcheck_context *context, size_t window_offset,
    const char *what, uint16_t channel,
    size_t last_window_sample, size_t first_error_sample, size_t last_error_sample)
{
    const double time = (1000.0L * first_error_sample) / context->fmt.sample_rate;
    if (first_error_sample == last_error_sample) {
        printf("%s: sample = %"PRIzu", time = %g ms", what, first_error_sample, time);
    }
    else {
        const double end_time = (1000.0L * last_error_sample) / context->fmt.sample_rate;
        printf("%s: samples = %"PRIzu" ... %"PRIzu" (%"PRIzu" samples, time = %g ms ... %g ms)",
            what, first_error_sample, last_error_sample, last_error_sample - first_error_sample + 1,
            time, end_time);
    }

    const size_t channels = context->fmt.channels;
    const size_t window_ints = context->window_size * channels;
    const size_t samples = last_window_sample >= context->window_size ?
        context->window_size : last_window_sample + 1;
    
    printf(", channel = %u, samples[%"PRIzu" ... %"PRIzu"] = {", channel,
        last_window_sample - samples + 1, last_window_sample);

    const size_t offset = (window_offset + channel + channels +
        (context->window_size - samples) * channels) % window_ints;
    int first = 1;
    for (size_t window_sample = 0; window_sample < samples; ++ window_sample)
    {
        const size_t i = (offset + window_sample * channels) % window_ints;
        if (first) {
            first = 0;
            printf("%d", context->window[i]);
        }
        else {
            printf(", %d", context->window[i]);
        }
    }

    printf("}\n");
}

void ripcheck_text_begin(
    void *data,
	const struct ripcheck_context *context)
{
    (void)data;
    printf("File: %s\n", context->filename);
    printf("[RIFF WAVE] %u bytes\n", context->riff_header.size);
    printf("[fmt ] %u bytes\n", context->riff_header.chunk.size);
    printf("  Audio format = %u (1 = PCM)\n", context->fmt.audio_format);
    printf("  Number of channels = %u (1 = mono, 2 = stereo)\n", context->fmt.channels);
    printf("  Sample rate = %uHz\n", context->fmt.sample_rate);
    printf("  Bytes / second = %u\n", context->fmt.byte_rate);
    printf("  Block alignment = %u\n", context->fmt.block_align);
    printf("  Bits / sample = %u\n", context->fmt.bits_per_sample);
}

void ripcheck_text_sample_data(
    void *data,
	const struct ripcheck_context *context,
    uint32_t data_size)
{
    (void)data;
    const double duration = (double)data_size / context->fmt.byte_rate;
    printf("[data] %u bytes\n", data_size);
    printf("  Duration = %g sec\n", duration);
}

void ripcheck_text_possible_pop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    uint16_t     channel,
    size_t       last_window_sample)
{
    (void)data;
    ripcheck_print_event(context, window_offset, "pop", channel, last_window_sample,
        context->poplocs[channel],  context->poplocs[channel]);
}

void ripcheck_text_possible_drop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    uint16_t     channel,
    size_t       last_window_sample,
    size_t       droped_sample)
{
    (void)data;
    ripcheck_print_event(context, window_offset, "drop", channel, last_window_sample,
        droped_sample, droped_sample);
}

void ripcheck_text_dupes(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    uint16_t     channel,
    size_t       last_window_sample)
{
    (void)data;
    ripcheck_print_event(context, window_offset, "dupes", channel, last_window_sample,
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
    else if (context->bad_areas == 1) {
        printf("done: 1 bad area found\n");
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
