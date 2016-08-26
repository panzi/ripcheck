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

#include "print_text.h"
#include "print_image.h"

#include <limits.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdarg.h>
#include <png.h>

#ifndef HAVE_STRLCPY
size_t strlcpy(char * dst, const char * src, size_t size);
#endif

static png_bytep *alloc_image(size_t width, size_t height)
{
    png_bytep *img = calloc(sizeof(png_bytep), height);

    if (!img) return NULL;

    size_t row_size = 3 * sizeof(png_byte) * width;
    for (size_t y = 0; y < height; ++ y) {
        png_bytep row = img[y] = malloc(row_size);

        if (!row) {
            for (size_t i = 0; i < y; ++ i) {
                free(img[i]);
            }
            free(img);
            return NULL;
        }
    }

    return img;
}

static void free_image(png_bytep *img, size_t height)
{
    for (size_t y = 0; y < height; ++ y) {
        free(img[y]);
    }
    free(img);
}

static void fill_rect(png_bytep *img,
    size_t x1, size_t y1, size_t x2, size_t y2,
    uint8_t color[3])
{
    uint8_t r = color[0];
    uint8_t g = color[1];
    uint8_t b = color[2];
    for (size_t y = y1; y <= y2; ++ y) {
        for (size_t x = x1; x <= x2; ++ x) {
            size_t i = 3 * x;
            png_bytep row = img[y];
            row[i + 0] = r;
            row[i + 1] = g;
            row[i + 2] = b;
        }
    }
}

static void write_image(const char *filename, png_bytep *img, size_t width, size_t height)
{
    FILE *fp = NULL;
    png_structp png = NULL;
    png_infop info  = NULL;

    fp = fopen(filename, "wb");

    if (!fp) {
        perror(filename);
        goto finalize;
    }

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png) {
        perror(filename);
        goto finalize;
    }

    info = png_create_info_struct(png);

    if (!info) {
        perror(filename);
        goto finalize;
    }

    if (setjmp(png_jmpbuf(png))) {
        perror(filename);
        goto finalize;
    }

    png_init_io(png, fp);

    // write header (8 bit color depth)
    png_set_IHDR(png, info, width, height,
        8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png, info);
    png_write_image(png, img);
    png_write_end(png, NULL);

    printf("written image: %s\n", filename);

finalize:
    if (fp)  fclose(fp);
    if (png) png_destroy_write_struct(&png, info ? &info : NULL);
}

static const char *basename(const char *path)
{
    const char *ptr = strrchr(path, '/');
#if defined(__WINDOWS__) || defined(__CYGWIN__)
    /* Windows supports both / and \ */
    const char *ptr2 = strrchr(path, '\\');
    if (ptr2 > ptr)
        ptr = ptr2;
#endif
    return ptr ? ptr + 1 : path;
}

typedef size_t (*filename_formatter_t)(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample);

static size_t format_errorname(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)context;
    (void)window_offset;
    (void)channel;
    (void)last_window_sample;
    (void)first_error_sample;
    (void)last_error_sample;

    return strlcpy(str, what, size);
}

static size_t format_filename(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)last_window_sample;
    (void)first_error_sample;
    (void)last_error_sample;

    return strlcpy(str, basename(context->filename), size);
}

static size_t format_filepath(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)last_window_sample;
    (void)first_error_sample;
    (void)last_error_sample;

    return strlcpy(str, context->filename, size);
}

static size_t format_basename(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)last_window_sample;
    (void)first_error_sample;
    (void)last_error_sample;

    if (size == 0)
        return 0;

    const char *name = basename(context->filename);
    const char *ptr = strrchr(name, '.');
    if (ptr && ptr != name) {
        const size_t n = ptr - name;
        const size_t m = n < size ? n : size - 1;
        memcpy(str, name, m);
        str[m] = 0;
        return n;
    }
    return strlcpy(str, name, size);
}

static size_t format_dirname(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)last_window_sample;
    (void)first_error_sample;
    (void)last_error_sample;

    const char *name = basename(context->filename);

    if (name == context->filename)
        return 0;

    size_t n = name - context->filename + 1; // + 1 for NUL
    return strlcpy(str, context->filename, n < size ? n : size);
}

static size_t format_channel(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)window_offset;
    (void)context;
    (void)what;
    (void)last_window_sample;
    (void)first_error_sample;
    (void)last_error_sample;

    int n = snprintf(str, size, "%u", channel);
    return n < 0 ? 0 : n;
}

static size_t format_first_error_sample(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)context;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)last_window_sample;
    (void)last_error_sample;

    int n = snprintf(str, size, "%"PRIzu, first_error_sample);
    return n < 0 ? 0 : n;
}

static size_t format_last_error_sample(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)context;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)last_window_sample;
    (void)first_error_sample;

    int n = snprintf(str, size, "%"PRIzu, last_error_sample);
    return n < 0 ? 0 : n;
}

static size_t format_error_samples(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)context;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)last_window_sample;

    int n = snprintf(str, size, "%"PRIzu, last_error_sample - first_error_sample + 1);
    return n < 0 ? 0 : n;
}

static size_t format_first_window_sample(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)first_error_sample;
    (void)last_error_sample;

    const size_t first_window_sample =
        context->window_size >= last_window_sample ?
        last_window_sample - context->window_size : 0;
    const int n = snprintf(str, size, "%"PRIzu, first_window_sample);
    return n < 0 ? 0 : n;
}

static size_t format_last_window_sample(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)context;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)first_error_sample;
    (void)last_error_sample;

    int n = snprintf(str, size, "%"PRIzu, last_window_sample);
    return n < 0 ? 0 : n;
}

static size_t format_window_size(
    char  *str,
    size_t size,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    (void)image_options;
    (void)window_offset;
    (void)what;
    (void)channel;
    (void)last_window_sample;
    (void)first_error_sample;
    (void)last_error_sample;

    int n = snprintf(str, size, "%"PRIzu, context->window_size);
    return n < 0 ? 0 : n;
}

struct filename_format_var {
    const char          *name;
    filename_formatter_t formatter;
};

static struct filename_format_var filename_format_vars[] = {
    {"errorname",           format_errorname},
    {"filename",            format_filename},
    {"filepath",            format_filepath},
    {"basename",            format_basename},
    {"dirname",             format_dirname},
    {"channel",             format_channel},
    {"first_error_sample",  format_first_error_sample},
    {"last_error_sample",   format_last_error_sample},
    {"error_samples",       format_error_samples},
    {"first_window_sample", format_first_window_sample},
    {"last_window_sample",  format_last_window_sample},
    {"window_size",         format_window_size},
    {0, 0}
};

static size_t format_image_filename(
    char        *str,
    size_t       size,
    const char  *format,
    const struct ripcheck_image_options *image_options,
    const struct ripcheck_context       *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    size_t n = 0;
    const char *from = format;
    const char *to   = NULL;
    
    while (*from) {
        to = from;
        // search varname start
        while (*to && *to != '{' && *to != '}') ++ to;

        if (from != to) {
            // non-format
            size_t slice = to - from;
            if (n < size) {
                size_t rem = size - n;
                size_t len = rem <= slice ? rem - 1 : slice;
                memcpy(str + n, from, len);
                str[n + len] = 0;
            }
            n += slice;
        }

        if (*to == '{') {
            if (to[1] == '{') {
                // "{{"
                if (n + 1 < size) {
                    str[n]     = '{';
                    str[n + 1] = 0;
                }
                ++ n;
                from = to + 2;
            }
            else {
                // "{varname}"
                from = to = to + 1;
                // search varname end
                while (*to && *to != '{' && *to != '}') ++ to;
                
                if (*to == '}') {
                    size_t keylen = to - from;
                    struct filename_format_var *fmtvar = filename_format_vars;

                    for (; fmtvar->name; ++ fmtvar)
                    {
                        if (strncmp(from, fmtvar->name, keylen) == 0)
                            break;
                    }

                    if (fmtvar->name) {
                        // print format
                        n += fmtvar->formatter(
                            str + n, size >= n ? size - n : 0, image_options, context,  window_offset, what,
                            channel, last_window_sample, first_error_sample, last_error_sample);
                        from = to + 1;
                    }
                    else {
                        // ignore unknown var name
                        size_t slice = keylen + 2;
                        if (n < size) {
                            size_t rem = size - n;
                            size_t len = rem <= slice ? rem - 1 : slice;
                            memcpy(str + n, from - 1, len);
                            str[n + len] = 0;
                        }
                        n += slice;
                        from = to + 1;
                    }
                }
                else {
                    // ignore error "{...{" or "{..."$
                    size_t slice = to - --from;
                    if (n < size) {
                        size_t rem = size - n;
                        size_t len = rem <= slice ? rem - 1 : slice;
                        memcpy(str + n, from, len);
                        str[n + len] = 0;
                    }
                    n += slice;
                    from = to;
                }
            }
        }
        else if (*to == '}') {
            if (n + 1 < size) {
                str[n]     = '}';
                str[n + 1] = 0;
            }
            ++ n;
            if (to[1] == '}') {
                // "}}"
                from = to + 2;
            }
            else {
                // ignore error "}..."
                from = to + 1;
            }
        }
        else {
            // end of string
            from = to;
        }
    }

    return n;
}

static void print_format_error(const char *format, size_t errpos, const char *msg, ...)
{
    va_list ap;
    fprintf(stderr, "error: illegal image filename format\n%s\n", format);
    for (size_t i = 0; i < errpos; ++ i) {
        fprintf(stderr, "-");
    }
    fprintf(stderr, "^\n");
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

int ripcheck_validate_image_filename_format(const char *format)
{
    const char *from = format;
    const char *to   = NULL;

    while (*from) {
        to = from;
        // search varname start
        while (*to && *to != '{' && *to != '}') ++ to;

        if (*to == '{') {
            if (to[1] == '{') {
                // "{{"
                from = to + 2;
            }
            else {
                // "{varname}"
                from = to = to + 1;
                // search varname end
                while (*to && *to != '{' && *to != '}') ++ to;

                if (*to == '}') {
                    size_t keylen = to - from;
                    struct filename_format_var *fmtvar = filename_format_vars;

                    for (; fmtvar->name; ++ fmtvar)
                    {
                        if (strncmp(from, fmtvar->name, keylen) == 0)
                            break;
                    }

                    if (fmtvar->name) {
                        // print format
                        from = to + 1;
                    }
                    else {
                        print_format_error(format, from - format, "unknown variable name: %.*s", keylen, from);
                        return EINVAL;
                    }
                }
                else if (*to == '{') {
                    print_format_error(format, to - format, "illegal character in variable name: '{'");
                    return EINVAL;
                }
                else {
                    print_format_error(format, from - format, "unterminated variable name");
                    return EINVAL;
                }
            }
        }
        else if (*to == '}') {
            if (to[1] == '}') {
                // "}}"
                from = to + 2;
            }
            else {
                print_format_error(format, to - format, "illegal lone '}'");
                return EINVAL;
            }
        }
        else {
            // end of string
            from = to;
        }
    }

    return 0;
}

static void print_image(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    const char  *what,
    uint16_t     channel,
    size_t last_window_sample,
    size_t first_error_sample,
    size_t last_error_sample)
{
    struct ripcheck_image_options *image_options =
        (struct ripcheck_image_options *)data;

    char filename[PATH_MAX];

    const size_t sample_height = image_options->sample_height;
    const size_t sample_width  = image_options->sample_width;

    const size_t channels = context->fmt.channels;
    const size_t window_ints = context->window_size * channels;
    const size_t samples = last_window_sample >= context->window_size ?
        context->window_size : last_window_sample + 1;

    const size_t zero   = sample_height + 1;
    const size_t height = sample_height * 2 + 1;
    const size_t width  = sample_width * samples;
    const int max_value = ~(~0 << (context->fmt.bits_per_sample - 1));

/*
    snprintf(filename, PATH_MAX, "%s_sample_%"PRIzu"_channel_%u_%s.png",
        basename(context->filename), first_error_sample, channel, what);
*/

    size_t namelen = format_image_filename(filename, PATH_MAX, image_options->filename, image_options,
        context, window_offset, what, channel, last_window_sample, first_error_sample, last_error_sample);

    if (namelen >= PATH_MAX) {
        fprintf(stderr, "error: image file name too long\n");
        return;
    }

    png_bytep *img = alloc_image(width, height);

    if (!img) {
        perror(filename);
        return;
    }

    fill_rect(img, 0, 0, width - 1, height - 1, image_options->bg_color);

    const size_t offset = (window_offset + channel + channels +
        (context->window_size - samples) * channels) % window_ints;
    for (size_t window_sample = 0; window_sample < samples; ++ window_sample)
    {
        const size_t i = (offset + window_sample * channels) % window_ints;
        const size_t x = window_sample * sample_width;
        const size_t sample = last_window_sample - samples + 1 + window_sample;
        int val = context->window[i] * (int)sample_height / max_value;
        uint8_t *color;
        if (sample >= first_error_sample && sample <= last_error_sample) {
            color = image_options->error_color;
            fill_rect(img, x, 0, x + sample_width - 1, height - 1, image_options->error_bg_color);
        }
        else {
            color = image_options->wave_color;
        }

        if (val < 0) {
            val = -val;
            fill_rect(img,
                x, zero,
                x + sample_width - 1,
                (unsigned int)val >= sample_height ? height - 1 : zero + val,
                color);
        }
        else {
            fill_rect(img,
                x, (unsigned int)val > zero ? 0 : zero - val,
                x + sample_width - 1, zero,
                color);
        }
    }

    fill_rect(img, 0, zero, width - 1, zero, image_options->zero_color);

    write_image(filename, img, width, height);

    free_image(img, height);
}

static int parse_color_channel(const char *str, uint8_t *compptr) {
    char ch = str[0];
    uint8_t comp = 0;
    if (ch >= 'a' && ch <= 'f')      comp = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F') comp = ch - 'A' + 10;
    else if (ch >= '0' && ch <= '9') comp = ch - '0';
    else return EINVAL;
    ch = str[1];
    comp *= 16;
    if (ch >= 'a' && ch <= 'f')      comp += ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F') comp += ch - 'A' + 10;
    else if (ch >= '0' && ch <= '9') comp += ch - '0';
    else return EINVAL;
    *compptr = comp;
    return 0;
}

struct color_def {
    const char *name;
    uint8_t color[3];
};

#define COLOR(col) {((col) >> 16) & 0xFF, ((col) >> 8) & 0xFF, (col) & 0xFF}

static struct color_def color_definitions[] = {
    {"black",   COLOR(0x000000)},
    {"silver",  COLOR(0xC0C0C0)},
    {"gray",    COLOR(0x808080)},
    {"white",   COLOR(0xFFFFFF)},
    {"maroon",  COLOR(0x800000)},
    {"red",     COLOR(0xFF0000)},
    {"purple",  COLOR(0x800080)},
    {"fuchsia", COLOR(0xFF00FF)},
    {"green",   COLOR(0x008000)},
    {"lime",    COLOR(0x00FF00)},
    {"olive",   COLOR(0x808000)},
    {"yellow",  COLOR(0xFFFF00)},
    {"navy",    COLOR(0x000080)},
    {"blue",    COLOR(0x0000FF)},
    {"teal",    COLOR(0x008080)},
    {"aqua",    COLOR(0x00FFFF)},
    {0,         COLOR(0x000000)}
};

static int parse_color(const char *restrict str, char **restrict endptr, uint8_t *color) {
    int errnum;
    while (isspace(*str)) ++ str;

    if (str[0] == '#') {
        ++ str;

        if ((errnum = parse_color_channel(str, &color[0])) != 0) return errnum;
        str += 2;
    
        if ((errnum = parse_color_channel(str, &color[1])) != 0) return errnum;
        str += 2;
    
        if ((errnum = parse_color_channel(str, &color[2])) != 0) return errnum;
        str += 2;
    }
    else {
        struct color_def *def = color_definitions;
        for (; def->name; ++ def) {
            size_t n = 0;
            while (isalnum(str[n])) ++ n;

            if (strncasecmp(def->name, str, n) == 0) {
                color[0] = def->color[0];
                color[1] = def->color[1];
                color[2] = def->color[2];
                str += n;
                break;
            }
        }
        if (!def->name) {
            return EINVAL;
        }
    }

    while (isspace(*str)) ++ str;
    if (endptr) *endptr = (char*)str;

    return 0;
}

static int parse_dim(const char *restrict str, char **restrict endptr, size_t *dim) {
    char *end = NULL;
    unsigned long long value = strtoull(str, &end, 10);
    if (end == str) return EINVAL;
    while (isspace(*end)) ++ end;
    if (value == 0 || value > SIZE_MAX) return EINVAL;
    if (endptr) *endptr = end;
    *dim = value;
    return 0;
}

// options syntax: KEY=VALUE[,KEY=VALUE]*
// Example:
// --visulaize=samp-width=20,samp-height=100,bg-color=white,wave-color=#0000FF,zero-color=gray,error-color=red,error-bg-color=#FFFF50
int ripcheck_parse_image_options(const char *str, struct ripcheck_image_options *image_options)
{
    if (*str == '\0') {
        return EINVAL;
    }

    while (*str) {
        const char *comma  = strchr(str, ',');
        const char *assign = strchr(str, '=');
        const char *value;
        const char *next;
        char *endptr = NULL;
        size_t keylen;
        int errnum = 0;

        if (!comma) {
            comma = str + strlen(str);
            next  = comma;
        }
        else {
            next  = comma + 1;
        }

        if (assign > comma || !assign) {
            return EINVAL;
        }
        else {
            value  = assign + 1;
            keylen = assign - str;
        }

        if (strncasecmp(str, "samp-width", keylen) == 0) {
            if ((errnum = parse_dim(value, &endptr, &image_options->sample_width)) != 0)
                return errnum;
        }
        else if (strncasecmp(str, "samp-height", keylen) == 0) {
            if ((errnum = parse_dim(value, &endptr, &image_options->sample_height)) != 0)
                return errnum;
        }
        else if (strncasecmp(str, "bg-color", keylen) == 0) {
            if ((errnum = parse_color(value, &endptr, image_options->bg_color)) != 0)
                return errnum;
        }
        else if (strncasecmp(str, "wave-color", keylen) == 0) {
            if ((errnum = parse_color(value, &endptr, image_options->wave_color)) != 0)
                return errnum;
        }
        else if (strncasecmp(str, "zero-color", keylen) == 0) {
            if ((errnum = parse_color(value, &endptr, image_options->zero_color)) != 0)
                return errnum;
        }
        else if (strncasecmp(str, "error-color", keylen) == 0) {
            if ((errnum = parse_color(value, &endptr, image_options->error_color)) != 0)
                return errnum;
        }
        else if (strncasecmp(str, "error-bg-color", keylen) == 0) {
            if ((errnum = parse_color(value, &endptr, image_options->error_bg_color)) != 0)
                return errnum;
        }
        else {
            return EINVAL;
        }

        if (endptr != comma) {
            return EINVAL;
        }

        str = next;
    }

    return 0;
}

void ripcheck_image_possible_pop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    uint16_t     channel,
    size_t       last_window_sample)
{
    ripcheck_text_possible_pop(data, context, window_offset, channel, last_window_sample);
    print_image(data, context, window_offset, "pop", channel, last_window_sample,
        context->poplocs[channel], context->poplocs[channel]);
}

void ripcheck_image_possible_drop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    uint16_t     channel,
    size_t       last_window_sample,
    size_t       droped_sample)
{
    ripcheck_text_possible_drop(data, context, window_offset, channel,
        last_window_sample, droped_sample);
    print_image(data, context, window_offset, "drop", channel,
        last_window_sample, droped_sample, droped_sample);
}

void ripcheck_image_dupes(
    void        *data,
    const struct ripcheck_context *context,
    size_t       window_offset,
    uint16_t     channel,
    size_t       last_window_sample)
{
    ripcheck_text_dupes(data, context, window_offset, channel, last_window_sample);
    print_image(data, context, window_offset, "dupes", channel, last_window_sample,
    context->dupelocs[channel], context->dupelocs[channel] + context->dupecounts[channel] - 1);
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
