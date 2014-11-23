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

#ifndef __GST_GVISION_PLUGIN_H__
#define __GST_GVISION_PLUGIN_H__

#include <gst/gst.h>
#include <stdint.h>

G_BEGIN_DECLS

#define FRAME_COUNTER 100U

enum pixels_type {
    PIXEL_YV12,
    PIXEL_RGB
};

/**
 * Color space format
 */
enum colors_type {
    COLOR_RGB,
    COLOR_HSV
};

/**
 * Buffer format
 */
struct image_format {
	uint32_t         width;
	uint32_t	     height;
	enum pixels_type pixelformat;
	uint32_t         bytesperline;
	uint32_t         size;
	enum colors_type colorspace;
};

/* #defines don't like whitespacey bits */
#define GVISION_BASE_TYPE \
  (gst_gvision_plugin_get_type())
#define GST_GVISION_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GVISION_BASE_TYPE,GstGVisionPlugin))
#define GST_GVISION_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GVISION_BASE_TYPE,GstGVisionPluginClass))
#define GST_IS_PLUGIN_TEMPLATE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GVISION_BASE_TYPE))
#define GST_IS_PLUGIN_TEMPLATE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GVISION_BASE_TYPE))

typedef struct _GstGVisionPlugin      GstGVisionPlugin;
typedef struct _GstGVisionPluginClass GstGVisionPluginClass;

struct _GstGVisionPlugin
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  gboolean silent;
};

struct _GstGVisionPluginClass
{
  GstElementClass parent_class;
};

GType gst_gvision_plugin_get_type (void);

G_END_DECLS

#endif /* __GST_GVISION_PLUGIN_H__ */
