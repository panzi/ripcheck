#ifndef RIPCHECK_PRINT_TEXT_H__
#define RIPCHECK_PRINT_TEXT_H__

#include "ripcheck.h"

extern struct ripcheck_callbacks ripcheck_callbacks_print_text;

void ripcheck_print_event(
    const struct ripcheck_context *context, const char *what, uint16_t channel,
    size_t last_window_sample, size_t first_error_sample, size_t last_error_sample);

void ripcheck_text_begin(
    void *data,
    const struct ripcheck_context *context);

void ripcheck_text_sample_data(
    void *data,
    const struct ripcheck_context *context,
    uint32_t data_size);

void ripcheck_text_possible_pop(
    void        *data,
    const struct ripcheck_context *context,
    uint16_t     channel,
    size_t       last_window_sample);

void ripcheck_text_possible_drop(
    void        *data,
    const struct ripcheck_context *context,
    uint16_t     channel,
    size_t       last_window_sample,
    size_t       droped_sample);

void ripcheck_text_dupes(
    void        *data,
    const struct ripcheck_context *context,
    uint16_t     channel,
    size_t       last_window_sample);

void ripcheck_text_complete(
    void *data,
    const struct ripcheck_context *context);

void ripcheck_text_error(
    void *data,
    const struct ripcheck_context *context,
    int errnum,
    const char *fmt, ...);

void ripcheck_text_warning(
    void *data,
    const struct ripcheck_context *context,
    const char *fmt, ...);

#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
