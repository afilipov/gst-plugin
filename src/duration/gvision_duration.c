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

#include "duration/gvision_duration.h"
#include "hashmap/cutils/hashmap.h"
#include "hashmap/gvision_hash.h"

#include <string.h>
#include <assert.h>

static Hashmap* reference_point = NULL;

static unsigned long symbols_hash(void *key)
{
    return sdbm_hash(key);
}

static bool str_icase_equals(void *keyA, void *keyB)
{
    return strcasecmp(keyA, keyB) == 0;
}

static bool remove_str_to_int(void *key, void *value, void *context)
{
    Hashmap* map = context;

    if (map) {
        hashmapRemove(map, key);
//        free(key);
        return true;
    }

    return false;
}

void prepare_duration_hashmaps(size_t initialCapacity)
{assert(initialCapacity);

    reference_point = hashmapCreate(initialCapacity, symbols_hash,
                                    str_icase_equals);
    assert(reference_point);
}

void release_duration_hashmaps()
{assert(reference_point);

    hashmapForEach(reference_point, remove_str_to_int, reference_point);
    hashmapFree(reference_point);
    reference_point = NULL;
}

unsigned long init_reference_point(const char* name, TimeNode_t* tn)
{assert(reference_point && name && tn);

    hashmapLock(reference_point);

    if (hashmapContainsKey(reference_point, (void*)name)) {

        struct timeval done;
        gettimeofday(&done, NULL);
        tn = hashmapGet(reference_point, (void*) name);
        tn->total_time = (double)(done.tv_usec - tn->init.tv_usec) /
                         1000000 + (double)(done.tv_sec - tn->init.tv_sec);
        tn->samples++;
    } else {

        tn = calloc(1, sizeof(TimeNode_t));
        tn->symbolic = strdup(name);
        gettimeofday(&tn->init, NULL);
        tn->samples = 0;
    }

    if (!hashmapPut(reference_point, (void*)(name), tn)) {
        printf("ERROR: %s(%d)\n", __func__, __LINE__);
    }

    hashmapUnlock(reference_point);

    return 0;
}

static bool show_duration(void* key, void* value, void* context)
{assert(key && value && context);

    TimeNode_t* tn = value;
    static unsigned int samples;

    if (!(samples++ % 11)) {
        printf("%s Duration:%f\n", tn->symbolic, tn->total_time / tn->samples);
    }

    return true;
}

void show_reference_delta()
{
    hashmapLock(reference_point);
    hashmapForEach(reference_point, show_duration, reference_point);
    hashmapUnlock(reference_point);
}

