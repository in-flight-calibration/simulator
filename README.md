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

## Docker

The image runs PX4 SITL, FlightGear (headless Xvfb) and the ROS bridge on the host network. It does not start a Zenoh router: PX4 and the ROS nodes connect to the router on the host at `localhost:7447`, so host ROS 2 applications see the simulator topics directly. Because the network is shared, PX4 MAVLink and FlightGear ports are the same as in a native run; do not run the native simulator at the same time.

Initialize submodules first (`git submodule update --init --recursive`); the image copies the PX4 checkout and its git metadata, but not `.env` or the host `build/`, `install/` and `log/` directories.

### Host router

Host and image must use the same ROS distribution (Lyrical), compatible `rmw_zenoh_cpp`, the same message definitions and the same `ROS_DOMAIN_ID` (PX4 `ZENOH_DOMAIN_ID` must match too). Run the router on the host, loopback only:
```
export RMW_IMPLEMENTATION=rmw_zenoh_cpp
ZENOH_CONFIG_OVERRIDE='listen/endpoints=["tcp/127.0.0.1:7447"]' ros2 run rmw_zenoh_cpp rmw_zenohd
```

### Build and run

All Docker files are in `docker/`. Build the image (rerun after source changes):
```
./docker/build.sh
```
Start the host router (see above), then the simulator; the script exits immediately when the router is not reachable on `localhost:7447`:
```
./simulator_docker.sh
```
Stop with `Ctrl+C` or `docker compose -f docker/compose.yaml down`.

`config.yaml` is mounted read-only from the workspace; edit it on the host and apply it with `docker compose -f docker/compose.yaml restart`. TerraSync scenery is cached in the `fgfs` volume.

PX4 retries every 5 s and ROS nodes reconnect if the host router restarts. Docker restarts the container only when the launch exits with an error. The container health status covers only PX4 and FlightGear; check topics from the host with `ros2 topic list`.

### GPU rendering

FlightGear always renders on the dedicated NVIDIA GPU through VirtualGL; the integrated GPU is not passed to the container. The host needs the NVIDIA driver and the [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html), configured for Docker:
```
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker
```
The container does not start without an NVIDIA GPU. Check the renderer in `/tmp/fgfs.log` inside the container.
