#!/bin/bash
set -e

if [[ $# -ne 1 || ! -d "$1" ]]; then
	echo "Usage: $0 <record>" >&2
	exit 1
fi

export RMW_IMPLEMENTATION=rmw_zenoh_cpp
source /opt/ros/$(ls /opt/ros | sort -V | tail -n 1)/setup.bash 
source install/setup.bash

ros2 run rmw_zenoh_cpp rmw_zenohd &
zenoh_pid=$!
cleanup() {
	kill "$zenoh_pid" 2>/dev/null || true
	wait "$zenoh_pid" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

ros2 bag play "$1" --loop
