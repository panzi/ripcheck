#include "print_text.h"
#include "print_image.h"

#include <limits.h>
#include <png.h>

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

// XXX: I think I draw "over the edges" somewhere -> crash
static void fill_rect(png_bytep *img,
    size_t x1, size_t y1, size_t x2, size_t y2,
    png_byte red, png_byte green, png_byte blue)
{
    for (size_t y = y1; y <= y2; ++ y) {
        for (size_t x = x1; x <= x2; ++ x) {
            size_t i = 3 * x;
            png_bytep row = img[y];
            row[i + 0] = red;
            row[i + 1] = green;
            row[i + 2] = blue;
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

    // Write header (8 bit colour depth)
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

static void print_image(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel,
    const char  *what,
    size_t       mark_start,
    size_t       mark_end)
{
    struct ripcheck_image_options *image_options =
        (struct ripcheck_image_options *)data;

    char filename[PATH_MAX];

    const size_t sample_height = image_options->sample_height;
    const size_t sample_width  = image_options->sample_width;

    const size_t zero   = sample_height + 1;
    const size_t height = sample_height * 2 + 1;
    const size_t width  = sample_width * context->window_size;
    const int max_value = ~(~0 << (context->fmt.bits_per_sample - 1));

    snprintf(filename, PATH_MAX, "%s_sample_%"PRIzu"_channel_%u_%s.png",
        basename(context->filename), sample, channel, what);

    png_bytep *img = alloc_image(width, height);

    if (!img) {
        perror(filename);
        return;
    }

    fill_rect(img, 0, 0, width - 1, height - 1, 255, 255, 255);

    for (size_t i = context->window_size; i > 0;) {
        size_t x = (context->window_size - i) * sample_width;
        int val = context->window[context->fmt.channels * --i + channel] * (int)sample_height / max_value;
        png_byte red = 0, green = 0, blue = 0;
        if (i >= mark_start && i <= mark_end) {
            red = 255;
            fill_rect(img, x, 0, x + sample_width - 1, height - 1, 255, 220, 96);
        }
        else {
            blue = 255;
        }

        if (val < 0) {
            val = -val;
            fill_rect(img,
                x, zero,
                x + sample_width - 1, val >= sample_height ? height - 1 : zero + val,
                red, green, blue);
        }
        else {
            fill_rect(img,
                x, val > zero ? 0 : zero - val,
                x + sample_width - 1, zero,
                red, green, blue);
        }
    }

    fill_rect(img, 0, zero, width - 1, zero, 127, 127, 127);

    write_image(filename, img, width, height);

    free_image(img, height);
}

void ripcheck_image_possible_pop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel)
{
    ripcheck_text_possible_pop(data, context, sample, channel);
    print_image(data, context, sample, channel, "pop", 2, 2);
}

void ripcheck_image_possible_drop(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel)
{
    ripcheck_text_possible_drop(data, context, sample, channel);
    print_image(data, context, sample, channel, "drop", 1, 1);
}

void ripcheck_image_dupes(
    void        *data,
    const struct ripcheck_context *context,
    size_t       sample,
    uint16_t     channel)
{
    ripcheck_text_dupes(data, context, sample, channel);
    print_image(data, context, sample, channel, "dupes", 1, context->dupecounts[channel]);
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
