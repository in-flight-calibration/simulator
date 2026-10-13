#!/bin/bash

set -e

source /opt/ros/$(ls /opt/ros | sort -V | tail -n 1)/setup.bash 
colcon build
source install/setup.bash
ros2 launch simulator launch.py