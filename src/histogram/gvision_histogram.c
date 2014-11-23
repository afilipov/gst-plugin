/**
 * Copyright (c) 2017 Atanas Filipov <it.feel.filipov@gmail.com>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "gvision_common.h"
#include "gvision_multithread.h"
#include "histogram/gvision_histogram.h"
#include "convert/gvision_convert.h"
#include "gnuplot/gvision_gnuplot.h"
#include "duration/gvision_duration.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>

/* TODO: */
#define MIN_PIXEL_VALUE 0
#define MAX_PIXEL_VALUE 255U

extern FILE *gplot_hd;

/**
 * You can use ${HONE}.gnuplot init file instead of this initialization
 */
static const char gnplot_init[] = {
   "set view 70, 45, 1, 1\n\
    set hidden3d\n\
    set dgrid3d 50,50 qnorm 2\n\
    splot '-' with lines\n"
};

static uint8_t active_pos = 0;

histo_ptr_t* data_array = NULL;

uint32_t cdf[MAX_HISTO_SIZE];

void prepare_histogram_array(unsigned int count)
{assert(count);

    data_array = malloc(count * sizeof(histo_ptr_t));
    if (!data_array) {
        fprintf(stderr, "Cannot allocate memory pool\n");
        exit(EXIT_FAILURE);
    }

    for (unsigned int idx = 0; idx < count; idx++) {
        data_array[idx] = malloc(MAX_HISTO_SIZE * sizeof(uint16_t));
        if (!data_array[idx]) {
            fprintf(stderr, "Cannot allocate memory chunk\n");
            exit(EXIT_FAILURE);
        }
    }
}

void release_histogram_array(unsigned int count)
{assert(count);

    if (!data_array) {
        fprintf(stderr, "Invalid memory pool\n");
    }
    for (unsigned int idx = 0; idx < count; idx++) {
        free(data_array[idx]);
    }
}

/* Draw histogram */
void plot_histograms(FILE* const fh, const uint16_t* const histogram,
                     uint16_t hsize)
{
    if (!fh) {
        return;
    }

    fprintf(fh, "%s", gnplot_init);
    for (unsigned int h = 0; h < HIST_COUNT; h += HIST_STEPS){
        for (unsigned int i = 0; i < hsize; i++){
            fprintf(fh, "%g %g %g\n", (double)i, (double)(h / HIST_STEPS),
                    (double)histogram[i]);
        }
    }

    fflush(fh);
    fprintf(fh, "e\n");
}

static void compute_cdf(uint32_t* cdf_table, uint16_t* pdf_table,
                        uint16_t hsize)
{assert(cdf_table && pdf_table && hsize);

    cdf_table[0] = pdf_table[0];
    /* Compute the Cumulative Distribution Function (cdf) */
    for (unsigned int i = 1; i < hsize; i++) {
        cdf_table[i] = cdf_table[i - 1] + pdf_table[i];
    }
}

static void normalize_cdf(uint32_t* cdf_table, uint16_t hsize,
                                    uint32_t divider)
{assert(cdf_table && hsize && divider);

    /* Normalize the cdf values */
    for (unsigned int i = 0; i < hsize; i++) {
        cdf_table[i] = i * ((double)cdf_table[i] / divider);
    }
}

void equalize_histogram(uint8_t* buf, const struct image_format* const fmt)
{assert(buf && fmt);

#ifdef CALC_TOTAL_DURATION
    /* start time */
    TimeNode_t point;
    point.symbolic = HOOK_ID;
    init_reference_point(point.symbolic, &point);
#endif
    uint8_t color;
    uint8_t* pixels = buf;
    uint8_t current_idx = active_pos++ % HIST_COUNT;
    uint16_t* used_histo = data_array[current_idx];

    memset(used_histo, 0, MAX_HISTO_SIZE * sizeof(*used_histo));

    /* Calculate and display histogram */
#ifdef MULTI_THREAD
    calc_histogram_pdf_mt(pixels, fmt, used_histo);
#else
    calc_histogram_pdf(pixels, fmt, used_histo);
#endif
    plot_histograms(gplot_hd, used_histo, MAX_HISTO_SIZE);

    /* Compute the CDF table */
    compute_cdf(cdf, used_histo, MAX_HISTO_SIZE);
    /* Normalize the CDF table */
    normalize_cdf(cdf, MAX_HISTO_SIZE, fmt->width * fmt->height);

    if (fmt->pixelformat == PIXEL_YV12) {
        /* Update pixels using equalized histogram */
        for (unsigned int h = 0; h < fmt->height; h++) {
            for (unsigned int w = 0; w < fmt->bytesperline; w++) {
                color = buf[w];
                buf[w] = cdf[color];
            }
            buf += fmt->bytesperline;
        }
    } else {
        /* Update pixels using equalized histogram */
        uint8_t bpp = fmt->bytesperline / fmt->width;
#ifdef CALC_TOTAL_DURATION
        /* start time */
        point.symbolic = HOOK_ID;
        init_reference_point(point.symbolic, &point);
#endif
        for (unsigned int h = 0; h < fmt->height; h++) {
            for (unsigned int w = 0; w < fmt->bytesperline; w += bpp) {
                rgb_t in;
                in.r = buf[w + 0]; in.g = buf[w + 1]; in.b = buf[w + 2];
                if (fmt->colorspace == COLOR_HSV) {
                    hsv_t out;
                    /* Convert pixel from RGB to HSV */
                    rgb2hsv(&in, &out);
                    /* Calcualte PDF for V only */
                    color = out.v * 255.0;
                    /* Update V */
                    out.v = cdf[color] / 255.0;
                    /* Convert pixel from HSV to RGB */
                    hsv2rgb(&out, &in);
                } else {
                    yuv_t out;
                    /* Convert pixel from RGB to YUV */
                    rgb2yuv(&in, &out);
                    /* Calcualte PDF for Y only */
                    color = out.y;
                    /* Update Y */
                    out.y = cdf[color];
                    /* Convert pixel from YUV to RGB */
                    yuv2rgb(&out, &in);
                }
                /* Update RGB */
                buf[w + 0] = in.r; buf[w + 1] = in.g; buf[w + 2] = in.b;
            }
            buf += fmt->bytesperline;
        }
#ifdef CALC_TOTAL_DURATION
        /* stop time */
        init_reference_point(point.symbolic, &point);
#endif
    }
#ifdef CALC_TOTAL_DURATION
    /* stop time */
    init_reference_point(point.symbolic, &point);
#endif
    show_reference_delta();
}

/* Compute the probability density functions (PDF) */
void calc_histogram_pdf(const uint8_t* const buf,
                        const struct image_format* const fmt, uint16_t* hresult)
{assert(buf && fmt && hresult);

    const uint8_t* pixels = buf;
    uint8_t color;
    uint8_t bpp = fmt->bytesperline / fmt->width;
#ifdef CALC_PDF_DURATION
    /* start time */
    TimeNode_t point;
    point.symbolic = HOOK_ID;
    init_reference_point(point.symbolic, &point);
#endif

#ifdef DEBUG
    printf("%s(%d) Offs:%p W:%d H:%d BPL:%d Size:%d\n",
       __func__, __LINE__, buf, fmt->width, fmt->height, fmt->bytesperline,
       fmt->size);
#endif
    /* calc current historgram */
    for (uint32_t h = 0; h < fmt->height; h++) {
        for (uint32_t w = 0; w < fmt->width; w += bpp) {
            if (fmt->pixelformat == PIXEL_YV12) {
                color = pixels[w];
            } else {
                if (fmt->colorspace == COLOR_HSV) {
                    rgb_t in;
                    hsv_t out;
                    /* Convert pixel from RGB to HSV */
                    in.r = pixels[w]; in.g = pixels[w + 1]; in.b = pixels[w + 2];
                    rgb2hsv(&in, &out);
                    /* Calcualte PDF for V only */
                    color = out.v * 255.0;
                } else {
                    rgb_t in;
                    yuv_t out;
                    /* Convert pixel from RGB to YUV */
                    in.r = pixels[w]; in.g = pixels[w + 1]; in.b = pixels[w + 2];
                    rgb2yuv(&in, &out);
                    /* Calcualte PDF for Y only */
                    color = out.y;
                }
            }
            hresult[color] += 1;
        }
        pixels += fmt->bytesperline;
    }
#ifdef CALC_PDF_DURATION
    /* stop time */
    init_reference_point(point.symbolic, &point);
#endif
}
