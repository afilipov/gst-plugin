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

#include "convert/gvision_convert.h"
#include "gvision_common.h"

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <float.h>
#include <math.h>

#define RGB_MIN   (0)
#define RGB_MAX (255)

static bool is_equal(double a, double b)
{
  return !!(fabsf(a - b) < FLT_EPSILON);
}

/**
 * H, S and V input  range = 0 - 1.0
 * R, G and B output range = 0 - 255
 */
void hsv2rgb(const hsv_t* const in, rgb_t* const out)
{assert(in && out);

    double pr, pg, pb, vh;

    if (is_equal(in->s, 0)) {
        /* Achromatic case, set level of grey */
        out->r = in->v * 255.0;
        out->g = in->v * 255.0;
        out->b = in->v * 255.0;
    } else {
        /* Determine levels of primary colours */
        if (in->h >= 1.0) {
            vh = 0.0;
        } else {
            vh = in->h * 6;
        }

        int vi = floor(vh);
        double p1 = in->v * (1.0 - in->s );
        double p2 = in->v * (1.0 - in->s * (vh - vi));
        double p3 = in->v * (1.0 - in->s * (1.0 - (vh - vi)));

        /* When vi > 0 vi < 6, 0 > H < 360 degrees */
        if (vi == 0) {        /* 0 > vi < 1,    0 > H <  60 degrees */
            pr = in->v;
            pg = p3;
            pb = p1;
        } else if (vi == 1) { /* 1 > vi < 2,   60 > H < 120 degrees */
            pr = p2;
            pg = in->v;
            pb = p1;
        } else if (vi == 2) { /* 2 > vi < 3,  120 > H < 180 degrees */
            pr = p1;
            pg = in->v;
            pb = p3;
        } else if (vi == 3) { /* 3 > vi < 4,  180 > H < 240 degrees */
            pr = p1;
            pg = p2;
            pb = in->v;
        } else if (vi == 4) { /* 4 > vi < 5,  240 > H < 300 degrees */
            pr = p3;
            pg = p1;
            pb = in->v;
        } else {              /* 5 > vi < 6,  300 > H < 360 degrees */
            pr = in->v;
            pg = p1;
            pb = p2;
        }

        /* Normalization */
        out->r = round(pr * 255.0);
        out->g = round(pg * 255.0);
        out->b = round(pb * 255.0);
    }
}

/**
 * R, G and B input range  = 0 - 255
 * H, S and V output range = 0 - 1.0
 */
void rgb2hsv(const rgb_t* const in, hsv_t* const out)
{assert(in && out);

    /* Color Min. value of RGB */
    unsigned char rgb_min = min(in->r, min(in->g, in->b));
    /* Color Max. value of RGB */
    unsigned char rgb_max = max(in->r, max(in->g, in->b));
    /* Delta calculation */
    unsigned char rgb_dlt = rgb_max - rgb_min;

    /* Calculate (V)alue */
    out->v = (double)rgb_max / 255.0;

    if (!rgb_dlt) {                         /* This is a gray */
        /* (H)ue is zero */
        out->h = 0;
        /* (S)aturation is zero */
        out->s = 0;
    } else {                                /* Chromatic data */
        double pr = (double)in->r / 255.0;
        double pg = (double)in->g / 255.0;
        double pb = (double)in->b / 255.0;

        /* Color Max */
        double pmax = (double)rgb_max / 255.0;
        /* Delta Max */
        double dmax = (double)rgb_dlt / 255.0;

        /* Calculate (S)aturation */
        out->s = dmax / pmax;
        double dr = (((pmax - pr ) / 6.0) + (dmax / 2.0)) / dmax;
        double dg = (((pmax - pg ) / 6.0) + (dmax / 2.0)) / dmax;
        double db = (((pmax - pb ) / 6.0) + (dmax / 2.0)) / dmax;

        /* Calculate (H)ue */
        if (is_equal(pr, pmax)) {
            out->h = db - dg;
        } else if (is_equal(pg, pmax)) {
            out->h = ( 1.0 / 3.0 ) + dr - db;
        } else if (is_equal(pb, pmax)) {
            out->h = ( 2.0 / 3.0 ) + dg - dr;
        }
    }
}

/**
 * Y, U and V input  range = 0 - 255
 * R, G and B output range = 0 - 255
 */
void yuv2rgb(const yuv_t* const in, rgb_t* const out)
{assert(in && out);

#ifdef EUROPE_EBU
 /**
  * European Y'U'V' (EBU)
  *
  * European TV (PAL and SECAM coded) uses Y'U'V' components. Y' is similar to
  * perceived luminance, U' and V' carry the colour information and some
  * luminance information and are bipolar (they go negative as well as positive)
  * The symbols U and V here are not related to the U and V of CIE YUV (1960).
  *
  * the coding equations for non-linear signals are:
  *
  *  R'= Y' + 1.140 * V'
  *  G'= Y' - 0.396 * U' - 0.581 * V'
  *  B'= Y' + 2.029 * U'
  */
    out->r = clamp(in->y + 1.140 * in->v, RGB_MIN, RGB_MAX);
    out->g = clamp(in->y - 0.396 * in->u - 0.581 * in->v, RGB_MIN, RGB_MAX);
    out->b = clamp(in->y + 2.029 * in->u, RGB_MIN, RGB_MAX);
#elif ITU_BT_601
 /**
  * ITU.BT-601 Y'CbCr
  *
  * This is the international standard for digital coding of TV pictures at 525
  * and 625 line rates. It is independent of the scanning standard and the
  * system primaries, therefore there are no chromaticity coordinates, no CIE
  * XYZ matrices, and no assumptions about white point or CRT gamma. It deals
  * only with the digital representation of R'G'B' signals in Y'CbCr form.
  * The non-linear coding matrices are:
  *
  *  Y' =  0.299 * R' + 0.587 * G' + 0.114 * B'
  *  Cb = -0.169 * R' - 0.331 * G' + 0.500 * B'
  *  Cr =  0.500 * R' - 0.419 * G' - 0.081 * B'
  *
  *  R' = Y' + 0.000 * U' + 1.403 * V'
  *  G' = Y' - 0.344 * U' - 0.714 * V'
  *  B' = Y' + 1.773 * U' + 0.000 * V'
  */
    out->r = clamp(in->y + 1.403 * in->v, RGB_MIN, RGB_MAX);
    out->g = clamp(in->y - 0.344 * in->u - 0.714 * in->v, RGB_MIN, RGB_MAX);
    out->b = clamp(in->y + 1.773 * in->u, RGB_MIN, RGB_MAX);
#elif ITU_BT_709
 /**
  * ITU.BT-709 HDTV studio production in Y'CbCr
  *
  * This is a recent standard, defined only as an interim standard for HDTV
  * studio production. It was defined by the CCIR (now the ITU) in 1988, but is
  * not yet recommended for use in broadcasting. The primaries are the R and B
  * from the EBU, and a G which is midway between SMPTE-C and EBU. The CRT gamma
  * law is assumed to be 2.2. White is D65.
  *
  * The coding equations for non-linear signals are:
  *
  *     R'= Y' + 0.0000*Cb + 1.5701*Cr
  *     G'= Y' - 0.1870*Cb - 0.4664*Cr
  *     B'= Y' - 1.8556*Cb + 0.0000*Cr
  */
    out->r = clamp(in->y + 1.5701 * in->v, RGB_MIN, RGB_MAX);
    out->g = clamp(in->y - 0.1870 * in->u - 0.4664 * in->v, RGB_MIN, RGB_MAX);
    out->b = clamp(in->y - 1.8556 * in->u, RGB_MIN, RGB_MAX);
#else
    int c = in->y -  16;
    int d = in->u - 128;
    int e = in->v - 128;

    out->r = clamp((298 * c + 409 * e + 128) >> 8, RGB_MIN, RGB_MAX);
    out->g = clamp((298 * c - 100 * d - 208 * e + 128) >> 8, RGB_MIN, RGB_MAX);
    out->b = clamp((298 * c + 516 * d + 128) >> 8, RGB_MIN, RGB_MAX);
#endif
}

/**
 * R, G and B input range  = 0 - 255
 * Y, U and V output range = 0 - 255
 */
void rgb2yuv(const rgb_t* in, yuv_t* out)
{assert(in && out);

#ifdef EUROPE_EBU
 /**
  * European Y'U'V' (EBU)
  *
  * European TV (PAL and SECAM coded) uses Y'U'V' components. Y' is similar to
  * perceived luminance, U' and V' carry the colour information and some
  * luminance information and are bipolar (they go negative as well as positive)
  * The symbols U and V here are not related to the U and V of CIE YUV (1960).
  *
  * the coding equations for non-linear signals are:
  *
  *  Y'=  0.299 * R' + 0.587 * G' + 0.114 * B'
  *  U'= -0.147 * R' - 0.289 * G' + 0.436 * B'
  *  V'=  0.615 * R' - 0.515 * G' - 0.100 * B'
  */
    out->y =  0.299 * in->r + 0.587 * in->g + 0.114 * in->b;
    out->u = -0.147 * in->r - 0.289 * in->g + 0.436 * in->b;
    out->v =  0.615 * in->r - 0.515 * in->g - 0.100 * in->b;
#elif ITU_BT_601
 /**
  * ITU.BT-601 Y'CbCr
  *
  * This is the international standard for digital coding of TV pictures at 525
  * and 625 line rates. It is independent of the scanning standard and the
  * system primaries, therefore there are no chromaticity coordinates, no CIE
  * XYZ matrices, and no assumptions about white point or CRT gamma. It deals
  * only with the digital representation of R'G'B' signals in Y'CbCr form.
  * The non-linear coding matrices are:
  *
  *  Y' =  0.299 * R' + 0.587 * G' + 0.114 * B'
  *  Cb = -0.169 * R' - 0.331 * G' + 0.500 * B'
  *  Cr =  0.500 * R' - 0.419 * G' - 0.081 * B'
  */
    out->y =  0.299 * in->r + 0.587 * in->g + 0.114 * in->b;
    out->u = -0.169 * in->r - 0.331 * in->g + 0.500 * in->b;
    out->v =  0.500 * in->r - 0.419 * in->g - 0.081 * in->b;
#elif ITU_BT_709
 /**
  * ITU.BT-709 HDTV studio production in Y'CbCr
  *
  * This is a recent standard, defined only as an interim standard for HDTV
  * studio production. It was defined by the CCIR (now the ITU) in 1988, but is
  * not yet recommended for use in broadcasting. The primaries are the R and B
  * from the EBU, and a G which is midway between SMPTE-C and EBU. The CRT gamma
  * law is assumed to be 2.2. White is D65.
  *
  * The coding equations for non-linear signals are:
  *
  *     Y'= 0.2215*R' + 0.7154*G' + 0.0721*B'
  *     Cb=-0.1145*R' - 0.3855*G' + 0.5000*B'
  *     Cr= 0.5016*R' - 0.4556*G' - 0.0459*B'
  */
    out->y =  0.2215 * in->r + 0.7154 * in->g + 0.0721 * in->b;
    out->u = -0.1145 * in->r - 0.3855 * in->g + 0.5000 * in->b;
    out->v =  0.5016 * in->r - 0.4556 * in->g - 0.0459 * in->b;
#else
    out->y = (( 66 * in->r + 129 * in->g +  25 * in->b + 128) >> 8) +  16;
    out->u = ((-38 * in->r -  74 * in->g + 112 * in->b + 128) >> 8) + 128;
    out->v = ((112 * in->r -  94 * in->g -  18 * in->b + 128) >> 8) + 128;
#endif
}

