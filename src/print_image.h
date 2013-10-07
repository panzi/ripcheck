#ifndef RIPCHECK_PRINT_IMAGE_H__
#define RIPCHECK_PRINT_IMAGE_H__

#include "ripcheck.h"

struct ripcheck_image_options {
    size_t sample_width;
    size_t sample_height;
};

int ripcheck_parse_image_options(
    const char *str,
    struct ripcheck_image_options *image_options);

void ripcheck_image_possible_pop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel);

void ripcheck_image_possible_drop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel);

void ripcheck_image_dupes(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel);

#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
