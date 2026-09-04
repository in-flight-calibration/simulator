#!/bin/bash

RESOLUTION="1280x720"
FOV="75"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

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
    --fg-aircraft="$SCRIPT_DIR/models" \
    --aircraft=uav \
    --timeofday=noon \
    --airport=EPBC \
    --runway=28L \
    --altitude=500 \
    --enable-terrasync \
    --disable-sound \
    --disable-random-objects \
    --prop:/sim/gui/menubar=false \
    --geometry="$RESOLUTION" \
    --fov="$FOV" \
    --prop:/sim/gui/menubar/autohide=true \
    --prop:/sim/traffic-manager/enabled=0 \
    --fdm=null \
    --max-fps=30 \
    > /dev/null 2>&1 &

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
        port=5004 \
        sync=false \
        async=false \
    > /dev/null 2>&1 &

GSTREAMER_PID=$!

echo "$GSTREAMER_PID" > "$STREAM_PID_FILE"
echo "GStreamer PID: $GSTREAMER_PID"

echo "FlightGear + GStreamer running. Waiting..."

wait