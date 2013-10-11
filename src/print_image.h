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
    uint8_t error_bg_color[3];
    const char *filename;
};

int ripcheck_parse_image_options(
    const char *str,
    struct ripcheck_image_options *image_options);

void ripcheck_image_possible_pop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    uint16_t     channel,
    size_t       last_window_sample);

void ripcheck_image_possible_drop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    uint16_t     channel,
    size_t       last_window_sample,
    size_t       droped_sample);

void ripcheck_image_dupes(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    uint16_t     channel,
    size_t       last_window_sample);

#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
