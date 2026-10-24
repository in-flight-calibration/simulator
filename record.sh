#!/bin/bash

set -e

export RMW_IMPLEMENTATION=rmw_zenoh_cpp
source /opt/ros/$(ls /opt/ros | sort -V | tail -n 1)/setup.bash 
source install/setup.bash

mkdir -p records
ros2 bag record -o records/`date +%Y%m%d_%H:%M:%S` -e '^/(fmu|flightgear).*'