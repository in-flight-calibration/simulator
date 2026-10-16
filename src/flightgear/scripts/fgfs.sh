#!/bin/bash

FGFS_WIDTH="${FGFS_WIDTH:-1280}"
FGFS_HEIGHT="${FGFS_HEIGHT:-720}"
FGFS_FOV="${FGFS_FOV:-75}"
FGFS_FPS="${FGFS_FPS:-30}"

FGFS_PORT_CONTROL="${FGFS_PORT_CONTROL:-5500}"
FGFS_PORT_VIDEO="${FGFS_PORT_VIDEO:-5501}"
FGFS_PORT_PROPS="${FGFS_PORT_PROPS:-5502}"

FGFS_PID_FILE="/tmp/fgfs.pid"
GSTREAMER_PID_FILE="/tmp/gst.pid"

FGFS_LOG_FILE="/tmp/fgfs.log"
GSTREAMER_LOG_FILE="/tmp/gst.log"

FGFS_PID=""
GSTREAMER_PID=""

cleanup() {
    echo "Shutting down..."

    if [ -n "$GSTREAMER_PID" ] && kill -0 "$GSTREAMER_PID" 2>/dev/null; then
        echo "Stopping GStreamer ($GSTREAMER_PID)"
        kill -INT "$GSTREAMER_PID" 2>/dev/null
    fi

    if [ -n "$FGFS_PID" ] && kill -0 "$FGFS_PID" 2>/dev/null; then
        echo "Stopping FlightGear ($FGFS_PID)"
        kill -TERM "$FGFS_PID" 2>/dev/null
    fi

    [ -n "$GSTREAMER_PID" ] && wait "$GSTREAMER_PID" 2>/dev/null
    [ -n "$FGFS_PID" ] && wait "$FGFS_PID" 2>/dev/null

    rm -f "$FGFS_PID_FILE" "$GSTREAMER_PID_FILE"

    echo "Shutdown complete"
    exit 0
}

trap cleanup SIGINT SIGTERM

fgfs \
    --log-level=info \
    --airport=EPBC --runway=28L \
    --generic=socket,in,$((2 * FGFS_FPS)),,"$FGFS_PORT_CONTROL",udp,control \
    --telnet="$FGFS_PORT_PROPS" \
    --fdm=null \
    --aircraft=uav \
    --timeofday=noon \
    --enable-terrasync \
    --geometry="${FGFS_WIDTH}x${FGFS_HEIGHT}" \
    --fov="$FGFS_FOV" \
    --max-fps="$FGFS_FPS" \
    --prop:/sim/gui/menubar=false \
    --prop:/sim/gui/menubar/autohide=true \
    --prop:/sim/menubar/visibility=false \
    --prop:/sim/traffic-manager/enabled=0 \
    --disable-sound \
    --disable-random-objects \
    > "$FGFS_LOG_FILE" 2>&1 &

FGFS_PID=$!

echo "$FGFS_PID" > "$FGFS_PID_FILE"
echo "FlightGear PID: $FGFS_PID"

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
    video/x-raw,framerate="$FGFS_FPS"/1 ! \
    x264enc tune=zerolatency \
        speed-preset=medium \
        bitrate=5000 \
        qp-min=18 \
        qp-max=30 ! \
    rtph264pay ! \
    udpsink host=239.255.0.1 \
        port="$FGFS_PORT_VIDEO" \
        auto-multicast=true \
        sync=false \
        async=false \
    > "$GSTREAMER_LOG_FILE" 2>&1 &

GSTREAMER_PID=$!

echo "$GSTREAMER_PID" > "$GSTREAMER_PID_FILE"
echo "GStreamer PID: $GSTREAMER_PID"

echo "FlightGear + GStreamer running. Waiting..."

wait
