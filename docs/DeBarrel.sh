#!/bin/sh
rm video00.mp4 video01.mp4 output.mp4
gst-launch-1.0 -e filesrc location=$1 ! decodebin name=decode ! videoscale ! 'video/x-raw,width=480,height=270' ! x264enc ! queue ! mp4mux name=mp4mux ! filesink location=video00.mp4 decode. ! audioconvert ! lamemp3enc bitrate=128 ! queue ! mp4mux.
gst-launch-1.0 -e filesrc location=$1 ! decodebin name=decode ! videoscale ! 'video/x-raw,width=480,height=270' ! videoconvert ! histogram ! videoconvert ! x264enc ! queue ! mp4mux name=mp4mux ! filesink location=video01.mp4 decode. ! audioconvert ! lamemp3enc bitrate=128 ! queue ! mp4mux.
ffmpeg -i video00.mp4 -i video01.mp4 -filter_complex "[0:v:0]pad=(iw*2)+10:ih:color=white[bg]; [bg][1:v:0]overlay=w+10" output.mp4
