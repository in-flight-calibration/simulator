# PX4-based ROS2 simulator for vision algorithms

## Features
* Semi-automated installation and automated launch
* Automated generation of ROS2 messages based on uORB topics.
* Automated estabilishion of Zenoh connection between PX4 and ROS2
* Visualization of flight with Flightgear and simulation of camera.

## Installation

### Prerequirements (not included in installation script)
* ROS2 (https://www.ros.org/)

### Installation script
Use dedicated script to install dependencies and configurate software.

Ubuntu:
```
./setup/ubuntu.sh
```

## Usage:
Use dedicated script to launch simulator.

Ubuntu:
```
./simulator.sh
```

To control the simulator use any Ground Control station: QGroundControl, Mission Planner. ApmPlanner etc.
The latest QGroundControl AppImage is downloaded with setup script.
