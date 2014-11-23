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

#include "defisheye/gvision_defisheye.h"
#include "gvision_common.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

/* http://mipav.cit.nih.gov/pubwiki/index.php/Barrel_Distortion_Correction
 *
 * Equation 1
 * M = a *r3_corr + b * r2_corr + c * r_corr + d
 * and
 * r_src = (a * r3_corr + b * r2_corr + c * r_corr + d) * r_corr
 * Where r_src and r_corr are specified in units of
 * the min((xDim-1)/2, (yDim-1)/2).
 *
 * Parameters in equation 1 are as follows:
 *  a, b and c describe distortion of the image
 *  d describes the linear scaling of the image
 *
 * Correcting using a affects only the outermost pixels of the image,
 * while b correction is more uniform. Using negative values for a, b, and
 * c shifts distant points away from the center. This counteracts barrel
 * distortion, and forms the basis for the above corrections.
 *
 * Using positive values for a, b, and c shifts distant points towards the
 * center. This counteracts pincushion distortion which is opposite to barrel
 * distortion.
 *
 * Using d=1, and a=b=c=0 leaves the image as it is. Choosing other d values
 * scales the image by that amount.
 * Correcting pincushion and/or barrel distortion
 *
 * Finally, you may correct pincushion and barrel distortions simultaneously
 * in the same image: if the outer regions exhibit barrel distortion, and the
 * inner parts pincushion, you should use negative a and positive b values.
 * If you do not want to scale the image, you should set d so that a+b+c+d = 1.
 */
unsigned char* calculate_defisheye(unsigned char *src, unsigned int ssize,
                                   const struct image_format const *fmt)
{assert(src && ssize && fmt);

    /* pointers to source pixels */
    const unsigned char* iYptr;
    const unsigned char* iUptr;
    const unsigned char* iVptr;

    /* pointers to destination pixels */
    unsigned char* oYptr;
    unsigned char* oUptr;
    unsigned char* oVptr;

    /* source pixel offset */
    unsigned int soffs;
    /* destination pixel offset */
    unsigned int doffs;

    /* Allocate output srcfer */
    GstBuffer *osrc = gst_buffer_new_allocate(NULL, ssize, NULL);
    /* Map output srcfer */
    GstMapInfo omap;
    if (!osrc || !gst_buffer_map(osrc, &omap, GST_MAP_WRITE)) {
        printf("Cannot map output srcfer\n");
        return NULL;
    }

    /* Get destination memory pointers */
    oYptr = omap.data;
    /* Get source memory pointers */
    iYptr = src;

    /* Check valid pointers to the memory */
    if (!oYptr || !iYptr) {
        printf("Invalid memory poiter In:%p Out:%p\n", iYptr, oYptr);
        return NULL;
    }

    if (fmt->pixelformat == PIXEL_YV12) {
        const unsigned int yplane_ssize = (fmt->width * fmt->height);
        const unsigned int uplane_ssize = (fmt->width >> 1) *
                                          (fmt->height >> 1);
        iUptr = iYptr + yplane_ssize;
        oUptr = oYptr + yplane_ssize;

        iVptr = iYptr + yplane_ssize + uplane_ssize;
        oVptr = oYptr + yplane_ssize + uplane_ssize;

    } else {

        iUptr = iYptr + 1;
        oUptr = oYptr + 1;

        iVptr = iUptr + 1;
        oVptr = oUptr + 1;
    }

    /*
     * a, b, c and FoV are physical properties of a lens/camera-combination
     * at a given focus distance. At different focus settings, FoV will
     * change noticeably, but usually it is fine to reuse a, b, and c even then.
     */

    float a = +0.000; /* affects only the outermost pixels of the image */
    float b = +0.076; /* most cases only require b optimization         */
    float c = +0.000; /* uniform correction                             */
    float d = +1.050; /* Scaling of the image                           */

    float deltaX, deltaY, dstR, srcR, factor, srcXd, srcYd = 0;

    float const centerX = (float)(fmt->width  >> 1);
    float const centerY = (float)(fmt->height >> 1);

    /* radius of the circle */
    unsigned int r = min(fmt->width, fmt->height) >> 1;

    for (unsigned int x = 0; x < fmt->width; x++) {
        for (unsigned int y = 0; y < fmt->height; y++) {

            /* cartesian coordinates of the destination point
             * (relative to the centre of the image)
             */
            deltaX = (x - centerX) / r;
            deltaY = (y - centerY) / r;

            /* distance or radius of destination image */
            dstR = sqrt(pow(deltaX, 2) + pow(deltaY, 2));

            /* distance or radius of src image (with formula)
             * r_src = (a * r3_corr + b * r2_corr + c * r_corr + d) * r_corr
             */
            srcR = (a * pow(dstR, 3) + b * pow(dstR, 2) + c * dstR + d) * dstR;

            /* comparing old and new distance to get factor */
            factor = fabs(dstR / srcR);

            /* coordinates in source image */
            srcXd = centerX + (deltaX * factor * r);
            srcYd = centerY + (deltaY * factor * r);

            /* Casting the float coordinates into int */
            unsigned int calcX = min((unsigned int)srcXd, fmt->width  - 1);
            unsigned int calcY = min((unsigned int)srcYd, fmt->height - 1);

            /* Source pixel offset */
            soffs = (calcY * fmt->width) + calcX;
            /* Destination pixel offset */
            doffs = (y * fmt->width) + x;

            /* Update YV12 or RGB planes */
            if (fmt->pixelformat == PIXEL_YV12) {
                /* Update Y plane */
                oYptr[doffs] = iYptr[soffs];

                /* Source pixel offset for U and V planes */
                soffs = ((calcY >> 1) * (fmt->bytesperline >> 1)) +
                         (calcX >> 1);
                /* Destination pixel offset for U and V planes*/
                doffs = ((y >> 1) * (fmt->bytesperline >> 1)) + (x >> 1);

                /* Update U plane */
                oUptr[doffs] = iUptr[soffs];
                /* Update V plane */
                oVptr[doffs] = iVptr[soffs];
            } else {
                uint8_t bpp = fmt->bytesperline / fmt->width;
                /* Update R plane */
                oYptr[doffs * bpp] = iYptr[soffs * bpp];
                /* Update G plane */
                oUptr[doffs * bpp] = iUptr[soffs * bpp];
                /* Update B plane */
                oVptr[doffs * bpp] = iVptr[soffs * bpp];
            }
        }
    }

    /* TODO: Remove copy */
    memcpy(src, omap.data, omap.size);

    /* Unmapping the output srcfer */
    if (oYptr != NULL) {
        gst_buffer_unmap(osrc, &omap);
        gst_buffer_unref(osrc);
    }

    return NULL;
}
