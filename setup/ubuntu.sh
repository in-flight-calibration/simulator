#!/bin/bash
set -e

git submodule update --init --recursive

sudo apt install -y \
    xvfb \
    x11-utils \
    mesa-utils \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav

wget -q -O- https://packagecloud.io/dcommander/virtualgl/gpgkey |
    gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/VirtualGL.gpg > /dev/null
sudo wget -q -O /etc/apt/sources.list.d/VirtualGL.list \
    https://raw.githubusercontent.com/VirtualGL/repo/main/VirtualGL.list
sudo apt update
sudo apt install -y virtualgl

if ! id -nG "$USER" | grep -qw render || ! id -nG "$USER" | grep -qw video; then
    sudo usermod -aG render,video "$USER"
    echo "Added $USER to render,video groups - log out and back in to apply"
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "Project directory: ${PROJECT_DIR}"

$PROJECT_DIR/src/flightgear/scripts/fgfs_install.sh
$PROJECT_DIR/src/px4/PX4-Autopilot/Tools/setup/ubuntu.sh || true # ignore errors

wget -O "$PROJECT_DIR/QGroundControl-x86_64.AppImage" https://github.com/mavlink/qgroundcontrol/releases/latest/download/QGroundControl-x86_64.AppImage
chmod +x "$PROJECT_DIR/QGroundControl-x86_64.AppImage"

source /opt/ros/$(ls /opt/ros | sort -V | tail -n 1)/setup.bash 
echo "Detected ROS2 distro: ${ROS_DISTRO}"

sudo rosdep init 2>/dev/null || true
rosdep update
rosdep install --from-paths "$PROJECT_DIR/src" --ignore-src -y --rosdistro "$ROS_DISTRO"
