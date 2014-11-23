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

#ifndef __GVISION_CONVERT_H__
#define __GVISION_CONVERT_H__

#include <stdint.h>

struct hsv_pixel {
    float h;
    float s;
    float v;
};
typedef struct hsv_pixel hsv_t;

struct rgb_pixel {
    int r;
    int g;
    int b;
};
typedef struct rgb_pixel rgb_t;

struct yuv_pixel {
    int y;
    int u;
    int v;
};
typedef struct yuv_pixel yuv_t;


/**
 * H, S and V input range  = 0 - 1.0
 * R, G and B output range = 0 - 255
 */
void hsv2rgb(const hsv_t* const in, rgb_t* const out);

/**
 * R, G and B input range  = 0 - 255
 * H, S and V output range = 0 - 1.0
 */
void rgb2hsv(const rgb_t* const in, hsv_t* const out);

/**
 * Y, U and V input  range = 0 - 255
 * R, G and B output range = 0 - 255
 */
void yuv2rgb(const yuv_t* const in, rgb_t* const out);

/**
 * R, G and B input range  = 0 - 255
 * Y, U and V output range = 0 - 255
 */
void rgb2yuv(const rgb_t* const in, yuv_t* const out);

#endif
