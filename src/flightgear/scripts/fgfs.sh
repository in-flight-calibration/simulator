#!/bin/bash

FGFS_WIDTH="${FGFS_WIDTH:-1280}"
FGFS_HEIGHT="${FGFS_HEIGHT:-720}"
FGFS_FOV="${FGFS_FOV:-75}"
FGFS_FPS="${FGFS_FPS:-30}"

FGFS_PORT_CONTROL="${FGFS_PORT_CONTROL:-5500}"
FGFS_PORT_VIDEO="${FGFS_PORT_VIDEO:-5501}"
FGFS_PORT_PROPS="${FGFS_PORT_PROPS:-5502}"

FGFS_MULTICAST_GROUP="${FGFS_MULTICAST_GROUP:-239.255.0.1}"
FGFS_MULTICAST_IFACE="${FGFS_MULTICAST_IFACE:-127.0.0.1}"

# 1 = render on a hidden Xvfb display, 0 = visible window on the current desktop
FGFS_HEADLESS="${FGFS_HEADLESS:-1}"
# VirtualGL device: egl, eglN or /dev/dri/cardN (see /opt/VirtualGL/bin/eglinfo -e)
FGFS_GPU_DEVICE="${FGFS_GPU_DEVICE:-egl}"

FGFS_PID_FILE="/tmp/fgfs.pid"
GSTREAMER_PID_FILE="/tmp/gst.pid"
XVFB_PID_FILE="/tmp/xvfb.pid"

FGFS_LOG_FILE="/tmp/fgfs.log"
GSTREAMER_LOG_FILE="/tmp/gst.log"
XVFB_LOG_FILE="/tmp/xvfb.log"

FGFS_PID=""
GSTREAMER_PID=""
XVFB_PID=""

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

    if [ -n "$XVFB_PID" ] && kill -0 "$XVFB_PID" 2>/dev/null; then
        echo "Stopping Xvfb ($XVFB_PID)"
        kill -TERM "$XVFB_PID" 2>/dev/null
        wait "$XVFB_PID" 2>/dev/null
    fi

    rm -f "$FGFS_PID_FILE" "$GSTREAMER_PID_FILE" "$XVFB_PID_FILE"

    echo "Shutdown complete"
    exit 0
}

trap cleanup SIGINT SIGTERM

start_xvfb() {
    local display_file
    display_file=$(mktemp)

    Xvfb -displayfd 3 \
        -screen 0 "${FGFS_WIDTH}x${FGFS_HEIGHT}x24" \
        -nolisten tcp \
        -noreset \
        3> "$display_file" \
        > "$XVFB_LOG_FILE" 2>&1 &

    XVFB_PID=$!
    echo "$XVFB_PID" > "$XVFB_PID_FILE"

    # Xvfb writes the display number once it is ready to accept clients
    local display_number=""
    for i in {1..50}; do
        display_number=$(head -n 1 "$display_file" 2>/dev/null)
        [ -n "$display_number" ] && break
        kill -0 "$XVFB_PID" 2>/dev/null || break
        sleep 0.1
    done
    rm -f "$display_file"

    if [ -z "$display_number" ]; then
        echo "ERROR: Xvfb failed to start (see $XVFB_LOG_FILE)"
        cleanup
    fi

    export DISPLAY=":$display_number"
    echo "Xvfb PID: $XVFB_PID, DISPLAY=$DISPLAY"
}

FGFS_LAUNCHER=()

if [ "$FGFS_HEADLESS" = "1" ]; then
    start_xvfb

    if command -v vglrun >/dev/null 2>&1; then
        FGFS_LAUNCHER=(vglrun -d "$FGFS_GPU_DEVICE")
        echo "Rendering on GPU via VirtualGL ($FGFS_GPU_DEVICE)"
    else
        echo "WARNING: vglrun not found, FlightGear will use software rendering (run setup/ubuntu.sh)"
    fi
fi

"${FGFS_LAUNCHER[@]}" fgfs \
    --log-level=info \
    --airport=EPBC --runway=28L \
    --generic=socket,in,$((2 * FGFS_FPS)),,"$FGFS_PORT_CONTROL",udp,control \
    --telnet="$FGFS_PORT_PROPS" \
    --fdm=null \
    --aircraft=uav \
    --timeofday=noon \
    --enable-terrasync \
    --geometry="${FGFS_WIDTH}x${FGFS_HEIGHT}" \
    --prop:/sim/startup/xpos=0 \
    --prop:/sim/startup/ypos=0 \
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

for i in {1..30}; do
    kill -0 "$FGFS_PID" 2>/dev/null || break

    FLIGHTGEAR_XID=$(xwininfo -root -tree |
        awk '/"FlightGear": \("OSG" "osgViewer"\)/ {print $1; exit}')

    [ -n "$FLIGHTGEAR_XID" ] && break

    sleep 1
done

if [ -z "$FLIGHTGEAR_XID" ]; then
    echo "ERROR: FlightGear OSG window not found (see $FGFS_LOG_FILE)"
    cleanup
fi

echo "FlightGear Window ID: $FLIGHTGEAR_XID"

start_gstreamer() {
    gst-launch-1.0 -q \
        ximagesrc display-name="$DISPLAY" \
        xid="$FLIGHTGEAR_XID" \
        show-pointer=false \
        use-damage=false ! \
        videoconvert ! \
        videorate ! \
        video/x-raw,framerate="$FGFS_FPS"/1 ! \
        x264enc tune=zerolatency \
            speed-preset=medium \
            bitrate=5000 \
            key-int-max="$FGFS_FPS" \
            qp-min=18 \
            qp-max=30 ! \
        rtph264pay config-interval=-1 pt=96 ! \
        udpsink host="$FGFS_MULTICAST_GROUP" \
            port="$FGFS_PORT_VIDEO" \
            auto-multicast=true \
            multicast-iface="$FGFS_MULTICAST_IFACE" \
            sync=false \
            async=false \
        >> "$GSTREAMER_LOG_FILE" 2>&1 &

    GSTREAMER_PID=$!
    echo "$GSTREAMER_PID" > "$GSTREAMER_PID_FILE"
    echo "GStreamer PID: $GSTREAMER_PID"
}

: > "$GSTREAMER_LOG_FILE"
start_gstreamer

echo "FlightGear + GStreamer running. Waiting..."

# ximagesrc dies with X BadMatch when the window goes (partially) off-screen
while kill -0 "$FGFS_PID" 2>/dev/null; do
    if ! kill -0 "$GSTREAMER_PID" 2>/dev/null; then
        echo "WARNING: GStreamer exited, restarting (see $GSTREAMER_LOG_FILE)"
        start_gstreamer
    fi
    sleep 1
done

echo "FlightGear exited"
cleanup
