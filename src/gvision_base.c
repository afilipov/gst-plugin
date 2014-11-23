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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/gstvalue.h>

#include "gvision_base.h"
#include "gvision_multithread.h"

#include "defisheye/gvision_defisheye.h"
#include "histogram/gvision_histogram.h"
#include "gnuplot/gvision_gnuplot.h"
#include "duration/gvision_duration.h"

#include <sys/time.h>

/* Filter signals and args */
enum {
  /* FILL ME */
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SILENT
};

/* TODO: */
FILE *gplot_hd = NULL;
static struct image_format bfmt;

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "video/x-raw,format=YV12;"
        "video/x-raw,format=RGB;")
   );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "video/x-raw,format=YV12;"
        "video/x-raw,format=RGB;")
    );

#define gst_gvision_plugin_parent_class parent_class
G_DEFINE_TYPE (GstGVisionPlugin, gst_gvision_plugin, GST_TYPE_ELEMENT);

static void gvision_get_video_caps(GstCaps *caps)
{
    const GstStructure *str;
    gint value;

    g_return_if_fail (gst_caps_is_fixed (caps));

    str = gst_caps_get_structure(caps, 0);
    if (!str) {
        GST_ERROR("Cannot get CAPS structure\n");
        return;
    }
    if (!gst_structure_get_int(str, "width", &value)) {
        GST_ERROR("No image width\n");
        return;
    }
    bfmt.width = value;

    if (!gst_structure_get_int(str, "height", &value)) {
        GST_ERROR("No image width\n");
        return;
    }
    bfmt.height = value;

    const gchar *fmt = gst_structure_get_string(str, "format");
    if (fmt == NULL) {
        GST_ERROR("Could not get format from src caps");
    }

    switch (GST_STR_FOURCC(fmt)) {
        case GST_MAKE_FOURCC('Y','V','1','2'):
            bfmt.pixelformat  = PIXEL_YV12;
            bfmt.bytesperline = bfmt.width;
            bfmt.size         = bfmt.height * bfmt.bytesperline;
            break;
        default:
            bfmt.pixelformat  = PIXEL_RGB;
            bfmt.bytesperline = bfmt.width * 3;
            bfmt.size         = bfmt.height * bfmt.bytesperline;
            bfmt.colorspace   = COLOR_RGB;
    }
    g_print("Width : %d\nHeight: %d Fmt:%d\n", bfmt.width, bfmt.height,
            bfmt.pixelformat);
}

static void
gst_gvision_plugin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGVisionPlugin *filter = GST_GVISION_PLUGIN (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_gvision_plugin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstGVisionPlugin *filter = GST_GVISION_PLUGIN (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* this function handles sink events */
static gboolean
gst_gvision_plugin_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
        fprintf(stderr, "data is to be discarded\n");
        break;
    case GST_EVENT_FLUSH_STOP:
        fprintf(stderr, "data is allowed again\n");
        break;
    case GST_EVENT_CAPS:
    {
        GstCaps * caps;
        gst_event_parse_caps (event, &caps);
        /* do something with the caps */

        /* and forward */
        fprintf(stderr, "Format information about the following buffers\n");
    }
        break;
    case GST_EVENT_SEGMENT:
        fprintf(stderr, "Timing information for the following buffers\n");
        break;
    case GST_EVENT_TAG:
        fprintf(stderr, "Stream metadata.\n");
        break;
    case GST_EVENT_BUFFERSIZE:
        fprintf(stderr, "Buffer size requirements\n");
        break;
    case GST_EVENT_SINK_MESSAGE:
        fprintf(stderr, "An event turned into a message by sinks\n");
        break;
    case GST_EVENT_EOS:
        fprintf(stderr, "no more data is to be expected on a pad.\n");
        release_histogram_pdf_mt();
        release_histogram_array(HIST_COUNT);
        release_duration_hashmaps();
        fprintf(stderr, "done\n");
        break;
    case GST_EVENT_QOS:
        fprintf(stderr, "A notification of the quality of service of the stream\n");
        break;
    case GST_EVENT_SEEK:
        fprintf(stderr, "A seek should be performed to a new position in the stream\n");
        break;
    case GST_EVENT_NAVIGATION:
        fprintf(stderr, "A navigation event.\n");
        break;
    case GST_EVENT_LATENCY:
        fprintf(stderr, "Configure the latency in a pipeline\n");
        break;
    case GST_EVENT_STEP:
        fprintf(stderr, "Stepping event\n");
        break;
    case GST_EVENT_RECONFIGURE:
        fprintf(stderr, "stream reconfigure event\n");
        break;

    default:
        printf("GenVision event %d\n", GST_EVENT_TYPE (event));
        break;
  }
  return gst_pad_event_default(pad, parent, event);
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_gvision_plugin_chain(GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstGVisionPlugin *filter = GST_GVISION_PLUGIN (parent);
    GstBuffer *buffer = buf;

    if (gplot_hd) {
        if (!bfmt.width || !bfmt.height) {
            GstCaps *vcaps = gst_pad_get_current_caps(pad);
            if (vcaps) {
                gvision_get_video_caps(vcaps);
                if (filter->silent == FALSE) {
                    g_print("GenVision plugged\n");
                }
            } else {
                g_print("Cannot get video properties\n");
            }
        }

        if (bfmt.width && bfmt.height && buffer) {
            /* Mapping a buffer */
            GstMapInfo map;
            if (gst_buffer_map(buf, &map, GST_MAP_WRITE)) {
#if 1
                uint8_t* pixels = map.data;
                if (pixels) {
                    equalize_histogram(pixels, &bfmt);
                }
#else
                /* Rectify distortion */
                calculate_defisheye(map.data, map.size, &bfmt);
#endif
            }
            gst_buffer_unmap(buf, &map);
        }
    }
    /* just push out the incoming buffer without touching it */
    return gst_pad_push(filter->srcpad, buf);
}

/* initialize the plugin's class */
static void
gst_gvision_plugin_class_init (GstGVisionPluginClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_gvision_plugin_set_property;
  gobject_class->get_property = gst_gvision_plugin_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "Image processing",
    "Filter/Converter/Video",
    "Visualization and Image processing element",
    "Atanas Filipov <it.feel.filipov@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_gvision_plugin_init (GstGVisionPlugin * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_gvision_plugin_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_gvision_plugin_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;

  prepare_duration_hashmaps(8192);

  prepare_histogram_array(HIST_COUNT);

  prepare_histogram_pdf_mt();

  gplot_hd = gnuplot_init() ;
}
