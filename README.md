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

### Camera optics

The `camera.*` parameters in `config.yaml` describe a pinhole camera with plumb_bob distortion:

| Parameter | Meaning |
|---|---|
| `width`, `height` | Published image size in pixels; `width` must be at least `height` |
| `fov` | Undistorted *horizontal* field of view in degrees; it sets the focal length (square pixels) |
| `cx`, `cy` | Principal point offset in pixels from the image center `((width - 1)/2, (height - 1)/2)`; positive moves right and down |
| `distortion.k1`, `k2`, `k3` | Radial coefficients (`k1 < 0` barrel, `k1 > 0` pincushion) |
| `distortion.p1`, `p2` | Tangential coefficients |

FlightGear renders an undistorted view centered on the optical axis with the same
focal length, enlarged as needed to cover the shifted and distorted image. The
video node then remaps it into the published size. With zero offsets and
distortion, the render equals the published image.

Strong barrel distortion has a hard limit: with `k1` only, the image corners must
satisfy `(2/3)/sqrt(-3 k1) >= r_corner`, where `r_corner` is the normalized corner
radius. For 1280x720 at 75 degrees this gives `k1 >= -0.19`; a smaller `fov` or a positive
`k2` allows more. Invalid optics stop the FlightGear node with an error.

### Record and replay

While the simulator is running, record the camera image and PX4 output topics into a rosbag:
```
./record.sh [output_path]
```
Without an argument the bag is saved to `records/<YYYYmmdd_HH:MM:SS>` next to the script. Pass `output_path` to override it.

To replay a recording (looped) run:
```
./replay.sh <record>
```
It starts its own Zenoh router, so stop the simulator first.

## Docker

The image runs PX4 SITL, FlightGear (headless Xvfb) and the ROS bridge on the host network. It does not start a Zenoh router: PX4 and the ROS nodes connect to the router on the host at `localhost:7447`, so host ROS 2 applications see the simulator topics directly. Because the network is shared, PX4 MAVLink and FlightGear ports are the same as in a native run; do not run the native simulator at the same time.

Initialize submodules first (`git submodule update --init --recursive`); the image copies the PX4 checkout and its git metadata and `.env` (the same variables as the native run; rebuild the image after changing it), but not the host `build/`, `install/` and `log/` directories.

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
