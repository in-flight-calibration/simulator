#!/bin/bash
set -e

git submodule update --init --recursive

apt install -ylibyaml-cpp-dev

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
