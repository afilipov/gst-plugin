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

#include "defisheye/gvision_defisheye.h"
#include "histogram/gvision_histogram.h"
#include "gnuplot/gvision_gnuplot.h"

GST_DEBUG_CATEGORY_STATIC (gst_gvision_plugin_debug);
#define GST_CAT_DEFAULT gst_gvision_plugin_debug

static gboolean plugin_init (GstPlugin * plugin)
{
  struct {
    const gchar *name;
    GType type;
  } *element, elements[] = {
    {"gvision", GVISION_BASE_TYPE},
    {NULL, 0},
  };

  for (element = elements; element->name; element++)
    if (!gst_element_register (plugin, element->name, GST_RANK_NONE,
            element->type))
      return FALSE;

  /**
   * debug category for fltering log messages
   */
  GST_DEBUG_CATEGORY_INIT (gst_gvision_plugin_debug, "gvision", 0,
                           "Genome Vision plugin");

  return TRUE;
}

/**
 * This is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h
 * GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "genome-vision"
#endif

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "genome-vision"
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "1.0.0"
#endif

#ifndef PACKAGE_STRING
#define PACKAGE_STRING "Genome Vision Image processing element"
#endif

#ifndef PACKAGE_URL
#define PACKAGE_URL "http://it-feel.com"
#endif

/**
 * gstreamer looks for this structure to register plugins
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR, GST_VERSION_MINOR, gvision, PACKAGE_STRING,
    plugin_init, PACKAGE_VERSION, "BSD", PACKAGE_NAME, PACKAGE_URL
)

