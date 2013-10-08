#ifndef RIPCHECK_PRINT_IMAGE_H__
#define RIPCHECK_PRINT_IMAGE_H__

#include "ripcheck.h"

struct ripcheck_image_options {
    size_t sample_width;
    size_t sample_height;
    uint8_t bg_color[3];
    uint8_t wave_color[3];
    uint8_t zero_color[3];
    uint8_t error_color[3];
    uint8_t hi_color[3];
};

int ripcheck_parse_image_options(
    const char *str,
    struct ripcheck_image_options *image_options);

void ripcheck_image_possible_pop(
    void        *data,
    const struct ripcheck_context *context,
    uint16_t     channel,
    size_t       last_window_sample);

void ripcheck_image_possible_drop(
    void        *data,
    const struct ripcheck_context *context,
    uint16_t     channel,
    size_t       last_window_sample,
    size_t       droped_sample);

void ripcheck_image_dupes(
    void        *data,
    const struct ripcheck_context *context,
    uint16_t     channel,
    size_t       last_window_sample);

#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
