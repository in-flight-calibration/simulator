#!/bin/bash

set -e

export RMW_IMPLEMENTATION=rmw_zenoh_cpp
source /opt/ros/$(ls /opt/ros | sort -V | tail -n 1)/setup.bash 
source install/setup.bash

mkdir -p records

ros2 bag record \
  -o "records/$(date +%Y%m%d_%H:%M:%S)" \
  /flightgear/image/compressed \
  /fmu/out/airspeed_validated \
  /fmu/out/sensor_combined \
  /fmu/out/sensor_gnss \
  /fmu/out/sensor_mag \
  /fmu/out/vehicle_acceleration \
  /fmu/out/vehicle_angular_velocity \
  /fmu/out/vehicle_attitude \
  /fmu/out/vehicle_attitude_groundtruth \
  /fmu/out/vehicle_global_position \
  /fmu/out/vehicle_global_position_groundtruth \
  /fmu/out/vehicle_gnss \
  /fmu/out/vehicle_local_position \
  /fmu/out/vehicle_status
