gstreamer-1.0 Experimental plugin including following partly developed functionality:
 - Histogram equalization
 - De-fisheye algorithm
 - Color converters, HSV2RGB, RGB2HSV, YUV2RGB, RGB2YUV
 - Hashmap supported different hash algorithms
 - Gnuplot utility to display sequnces of calculated histograms

How to try:

gst-launch-1.0 -v v4l2src device=/dev/video0 extra-controls="c,exposure_auto=1" ! gvision ! videoconvert ! ximagesink sync=false 

or

gst-launch-1.0 videotestsrc ! video/x-raw,framerate=30/1,width=320,height=240 ! gvision ! videoconvert ! ximagesink sync=false

The plugin still under development !!!
