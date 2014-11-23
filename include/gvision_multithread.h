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

#ifndef __GVISION_MULTITHREAD_H__
#define __GVISION_MULTITHREAD_H__

#include "gvision_base.h"
#include <pthread.h>
#include <stdbool.h>

enum thread_state {
    STANDBY,
    WORKING
};

struct thread_context {
    int id;
    int nproc;
    const uint8_t* offset;
    uint16_t* results;
    struct image_format format;
    enum thread_state state;
#ifdef CALC_THREAD_DURATION
    double sum;
    unsigned int counter;
    struct timeval start;
    struct timeval stop;
#endif
};
typedef struct thread_context tcontext_t;

void prepare_histogram_pdf_mt(void);

void calc_histogram_pdf_mt(const uint8_t* const buf,
                           const struct image_format* const fmt,
                           uint16_t* const hresult);

void release_histogram_pdf_mt(void);

#endif
