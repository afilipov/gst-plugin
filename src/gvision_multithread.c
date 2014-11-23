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

#include "gvision_multithread.h"
#include "gvision_common.h"
#include "histogram/gvision_histogram.h"
#include "duration/gvision_duration.h"
#include "hashmap/gvision_hash.h"

#ifdef FREEBSD
#include <sys/cpuset.h>
#else
#include <sched.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/time.h>

tcontext_t*     ctx;
pthread_t*      threads;
pthread_attr_t* pthread_attr;

unsigned int cpus = 1;
bool do_processing = true;

pthread_cond_t  wait_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t wait_lock = PTHREAD_MUTEX_INITIALIZER;

static void* histogram_calculating_thread(void *arg)
{
    tcontext_t *tctx = (tcontext_t *) arg;

    while(do_processing) {

        pthread_mutex_lock(&wait_lock);
        pthread_cond_wait(&wait_cond, &wait_lock);
        pthread_mutex_unlock(&wait_lock);

        /* start time */
        TimeNode_t point;
        point.symbolic = HOOK_ID;
        init_reference_point(point.symbolic, &point);

#ifdef MTHREAD_DEBUG
        printf("%s(%d)ID:%d Offs:%p W:%d H:%d BPL:%d HPtr:%p CPU:%d\n",
               __func__, __LINE__, tctx->id, tctx->offset, tctx->format.width,
               tctx->format.height, tctx->format.bytesperline, tctx->results,
               sched_getcpu());
#endif

        calc_histogram_pdf(tctx->offset, &tctx->format, tctx->results);

#ifdef RANDOM_LAG
        /* Functionality check */
        long int rnd = random() % 140000;
        usleep(rnd);
#endif
        tctx->state = STANDBY;

        /* stop time */
        init_reference_point(point.symbolic, &point);
    }
	return NULL;
}

void prepare_histogram_pdf_mt()
{
    unsigned int piece;
    cpus = sysconf(_SC_NPROCESSORS_CONF);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    threads = (pthread_t *) malloc(cpus * sizeof(*threads));
    pthread_attr = (pthread_attr_t *) malloc(cpus * sizeof(*pthread_attr));
    ctx = (tcontext_t*) calloc(cpus, sizeof(tcontext_t));

    do_processing = true;

    /* Create and setup each thread. */
    for (piece = 0; piece < cpus; piece++) {
        /* Initialize thread creation attributes */
        CPU_SET(piece, &cpuset);
        if (pthread_attr_init(&pthread_attr[piece])) {
            perror("Cannon init pthread attributes.");
        }
        pthread_attr_setaffinity_np(&pthread_attr[piece], sizeof(cpuset),
                                    &cpuset);
        pthread_create(&threads[piece], &pthread_attr[piece],
                       histogram_calculating_thread, &ctx[piece]);
    }
    srandom(time(NULL));
}

void release_histogram_pdf_mt()
{
    unsigned int piece;

    do_processing = false;
    pthread_cond_broadcast(&wait_cond);

    /* Synchronize the completion of each thread. */
    for (piece = 0; piece < cpus; piece++) {
        void *result;
        if (pthread_join(threads[piece], &result)) {
            perror("Cannon join pthread.");
        }
        if (result) {
            printf("pthread %d joined with returned value is %s\n",
                   ctx[piece].id, (char*) result);
        }

        /* Destroy the thread attributes object */
        if (pthread_attr_destroy(&pthread_attr[piece])) {
            perror("Cannon destroy pthread attributes.");
        }

        /* Free memory allocated by thread */
        free(result);
    }

    free(threads);
    free(pthread_attr);
    free(ctx);
}

void calc_histogram_pdf_mt(const uint8_t* const buf,
                           const struct image_format* const fmt,
                           uint16_t* const hresult)
{assert(buf && fmt && hresult);

#ifdef CALC_TOTAL_DURATION
    /* start time */
    TimeNode_t point;
    point.symbolic = HOOK_ID;
    init_reference_point(point.symbolic, &point);
#endif
    unsigned int piece;
    uint32_t piece_height = fmt->height / cpus;
    uint32_t piece_size   = fmt->size   / cpus;
    uint32_t lines_rest   = fmt->height % cpus;

	/* Start up thread */
    for (piece = 0; piece < cpus; piece++) {
		ctx[piece].id = piece;
		ctx[piece].results = hresult;
        ctx[piece].state = WORKING;

        /* Propagate buffer offset */
        ctx[piece].offset = &buf[piece_height * piece * fmt->bytesperline];

        /* Propagate parameters */
        ctx[piece].format = *fmt;
        /* Overwrite hight and size */
        ctx[piece].format.height = piece_height;
        ctx[piece].format.size   = piece_size;

        /* In case of odd thread count, add rest of lines to first thread */
        if (lines_rest && !piece) {
            /* Adding rest of lines */
            ctx[piece].format.height += fmt->height % cpus;
            /* Recalculate the size */
            ctx[piece].format.size = piece_height * fmt->bytesperline;
        }
	}

    pthread_cond_broadcast(&wait_cond);
    for (piece = 0; piece < cpus; piece++) {
        while(ctx[piece].state != STANDBY) {
            sched_yield();
        };
    }
#ifdef CALC_TOTAL_DURATION
    /* stop time */
    init_reference_point(point.symbolic, &point);
#endif
}

