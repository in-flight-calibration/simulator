#!/bin/bash

RESOLUTION="1280x720"
FOV="75"
FPS="30"

PORT_CONTROL=5500
PORT_VIDEO=5501

FGFS_PID_FILE="/tmp/fgfs.pid"
STREAM_PID_FILE="/tmp/fgfs_stream.pid"

FLIGHTGEAR_PID=""
GSTREAMER_PID=""

cleanup() {
    echo "Shutting down..."

    if [ -n "$GSTREAMER_PID" ] && kill -0 "$GSTREAMER_PID" 2>/dev/null; then
        echo "Stopping GStreamer ($GSTREAMER_PID)"
        kill -INT "$GSTREAMER_PID" 2>/dev/null
    fi

    if [ -n "$FLIGHTGEAR_PID" ] && kill -0 "$FLIGHTGEAR_PID" 2>/dev/null; then
        echo "Stopping FlightGear ($FLIGHTGEAR_PID)"
        kill -TERM "$FLIGHTGEAR_PID" 2>/dev/null
    fi

    [ -n "$GSTREAMER_PID" ] && wait "$GSTREAMER_PID" 2>/dev/null
    [ -n "$FLIGHTGEAR_PID" ] && wait "$FLIGHTGEAR_PID" 2>/dev/null

    rm -f "$FGFS_PID_FILE" "$STREAM_PID_FILE"

    echo "Shutdown complete"
    exit 0
}

trap cleanup SIGINT SIGTERM

fgfs \
    --log-level=info \
    --airport=EPBC --runway=28L \
    --generic=socket,in,30,,5500,udp,control \
    --fdm=null \
    --aircraft=uav \
    --timeofday=noon \
    --enable-terrasync \
    --geometry="$RESOLUTION" \
    --fov="$FOV" \
    --max-fps=$FPS \
    --prop:/sim/gui/menubar=false \
    --prop:/sim/gui/menubar/autohide=true \
    --prop:/sim/traffic-manager/enabled=0 \
    --disable-sound \
    --disable-random-objects \
    > fg.log 2>&1 &

FLIGHTGEAR_PID=$!

echo "$FLIGHTGEAR_PID" > "$FGFS_PID_FILE"
echo "FlightGear PID: $FLIGHTGEAR_PID"

FLIGHTGEAR_XID=""

for i in {1..5}; do
    FLIGHTGEAR_XID=$(xwininfo -root -tree |
        awk '/"FlightGear": \("OSG" "osgViewer"\)/ {print $1; exit}')

    [ -n "$FLIGHTGEAR_XID" ] && break

    sleep 1
done

if [ -z "$FLIGHTGEAR_XID" ]; then
    echo "ERROR: FlightGear OSG window not found"
    cleanup
fi

echo "FlightGear Window ID: $FLIGHTGEAR_XID"

gst-launch-1.0 -q \
    ximagesrc xid="$FLIGHTGEAR_XID" \
    show-pointer=false \
    use-damage=false ! \
    videoconvert ! \
    videorate ! \
    video/x-raw,framerate=15/1 ! \
    x264enc tune=zerolatency \
        speed-preset=medium \
        bitrate=5000 \
        qp-min=18 \
        qp-max=30 ! \
    rtph264pay ! \
    udpsink host=127.0.0.1 \
        port="$PORT_VIDEO" \
        sync=false \
        async=false \
    > /dev/null 2>&1 &

GSTREAMER_PID=$!

echo "$GSTREAMER_PID" > "$STREAM_PID_FILE"
echo "GStreamer PID: $GSTREAMER_PID"

echo "FlightGear + GStreamer running. Waiting..."

wait