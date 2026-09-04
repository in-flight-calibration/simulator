#!/bin/bash

gst-launch-1.0 udpsrc port=5004 ! application/x-rtp,media=video,encoding-name=H264,payload=96 ! rtph264depay ! avdec_h264 ! videoconvert ! fpsdisplaysink video-sink=autovideosink text-overlay=true
