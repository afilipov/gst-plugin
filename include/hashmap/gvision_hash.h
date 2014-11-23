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

#ifndef __GVISION_HASHING_H__
#define __GVISION_HASHING_H__

/**
 * DJB2 - "http://www.cse.yorku.ca/~oz/hash.html"
 * This algorithm (k=33) was first reported by dan bernstein many years ago
 * in comp.lang.c. another version of this algorithm (now favored by bernstein)
 * uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33
 * (why it works better than many other constants, prime or not) has never been
 * adequately explained.
 */
unsigned long djb2_hash(const char *str);

/**
 * SDBM - "http://www.cse.yorku.ca/~oz/hash.html"
 * This algorithm was created for sdbm (a public-domain reimplementation of
 * ndbm) database library. it was found to do well in scrambling bits, causing
 * better distribution of the keys and fewer splits. it also happens to be a
 * good general hashing function with good distribution. the actual function
 * is hash(i) = hash(i - 1) * 65599 + str[i]; what is included below is the
 * faster version used in gawk. [there is even a faster, duff-device version]
 * the magic constant 65599 was picked out of thin air while experimenting
 * with different constants, and turns out to be a prime. this is one of the
 * algorithms used in berkeley db (see sleepycat) and elsewhere.
 */
unsigned long sdbm_hash(const char *str);

/**
 * https://en.wikipedia.org/wiki/Jenkins_hash_function
 * The Jenkins hash functions are a collection of (non-cryptographic) hash
 * functions for multi-byte keys designed by Bob Jenkins. The first one was
 * formally published in 1997.
 */
unsigned long time_hash(const char *str);

/**
 * http://www.cs.yale.edu/homes/aspnes/pinewiki/ \
 * C(2f)HashTables.html?highlight=%28CategoryAlgorithmNotes%29
 *
 * Implements universal hashing using random bit-vectors in x
 * assumes number of elements in x is at least BITS_PER_CHAR * MAX_STRING_SIZE
 */
unsigned long rand_hash(const char *s, unsigned long x[]);

#endif
