# Simulator

Links ROS 2 with FlightGear.

## Installation

Choose one of the following setup paths:

- [Install and run with Docker](#with-docker) — uses the provided [Dockerfile](Dockerfile) with ROS 2 Rolling and the CUDA toolkit.
- [Install and run natively, without Docker](#without-docker) — install ROS 2 Rolling and the project dependencies directly on the host.

Both paths use the same source workspace and require FlightGear. See [Install FlightGear](#install-flightgear).

### With Docker

The commands below assume you are in the workspace root: the directory containing the `Dockerfile` and `src/` directory.

Build the image:

```bash
sudo docker build -t flight_ros .
```

If you want to use the FlightGear GUI, allow local root containers to connect to your X11 display:

```bash
xhost +local:root
```

Start a shell in the container with the workspace bind-mounted at `/ros_ws`:

```bash
sudo docker run --rm -it --gpus all \
  -e DISPLAY="$DISPLAY" \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  -v "$PWD":/ros_ws \
  -w /ros_ws \
  flight_ros bash
```

The host needs the NVIDIA Container Toolkit configured for Docker to use `--gpus all`. If it is not configured, remove `--gpus all`; the container will run without GPU access. The complete simulation still requires the `DISPLAY` environment variable and X11 socket mount because it captures the FlightGear window.

Inside the container, install ROS dependencies and build:

```bash
source /opt/ros/rolling/setup.bash
apt-get update
rosdep update
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

Run `apt-get update` before `rosdep install`: the Dockerfile removes apt package indexes after installing packages to reduce image size.

The workspace is bind-mounted, so source changes are visible on both the host and container. Files generated in the workspace, such as the `build/`, `install/`, and `log/` directories, also remain on the host after the container exits.

After closing the container, revoke the X11 permission on the host if you enabled it:

```bash
xhost -local:root
```

### Without Docker

Install [ROS 2 Rolling](https://docs.ros.org/en/rolling/Installation.html) on the host, following the instructions for your Ubuntu version. Then, from your ROS workspace root (the directory containing `src/`), source ROS and install `rosdep` if needed, initialize it, and install the package dependencies:

```bash
source /opt/ros/rolling/setup.bash
sudo apt update
sudo apt install python3-rosdep
sudo rosdep init
rosdep update
rosdep install --from-paths src --ignore-src -r -y
```

If `rosdep` is already initialized on your system, skip `sudo rosdep init`. Build and source the workspace:

```bash
colcon build --symlink-install
source install/setup.bash
```

Install CUDA separately on the host only if your own GPU-enabled dependencies or applications require it; the Docker image's CUDA installation does not apply to a native installation.

### Install FlightGear

Reccomended option to install [flightgear.org](https://www.flightgear.org/) is to do it with script provided in `flightgear` dir:

```bash
source ./src/flightgear/scripts/fgfs_install.sh
```

## Usage

After building the workspace, open a terminal in the workspace root and source both ROS 2 and the workspace overlay:

```bash
source /opt/ros/rolling/setup.bash
source install/setup.bash
```

Start the complete simulation with the launch file provided by the `dynamics` package:

```bash
ros2 launch dynamics launch.py
```

This starts the FlightGear integration, aircraft dynamics simulation, sensor simulation, and control system. The control panel lets you select `IDLE`, `TAKEOFF`, `CRUISE`, or `SLALOM` and set target height and speed. 

### ROS topics

The launch starts these publishers. Custom message types are defined in the `aircraft_msgs` package.

| Topic | Message type | Published data |
|---|---|---|
| `/dynamics/groundtruth` | `aircraft_msgs/msg/Groundtruth` | Simulated aircraft state: position, orientation, linear and angular velocity, acceleration, airspeed, simulation time, and latitude/longitude/altitude. Consumed by the sensor, control, and FlightGear nodes. |
| `/environment` | `aircraft_msgs/msg/Environment` | Simulated atmospheric pressure, temperature, air density, and wind. Consumed by the sensor node. |
| `/sensors/air` | `aircraft_msgs/msg/SensorAir` | Simulated air-data readings: static pressure, differential pressure, and temperature. |
| `/sensors/gnss` | `aircraft_msgs/msg/SensorGnss` | Simulated GNSS readings: latitude, longitude, altitude, ground speed, course, and vertical speed. |
| `/sensors/imu` | `aircraft_msgs/msg/SensorImu` | Simulated 9-DOF IMU readings: accelerometer, gyroscope, and magnetometer vectors. |
| `/sensors/ins` | `aircraft_msgs/msg/SensorIns` | Simulated INS orientation quaternion estimate. |
| `/dynamics/control` | `aircraft_msgs/msg/Control` | Controller outputs: aileron, elevator, rudder, and throttle commands. The dynamics node consumes these commands. |
| `/control/state` | `aircraft_msgs/msg/ControlState` | Controller state and diagnostics, including target speed/height, errors, desired attitude/rates, and integrals. |
| `/control/set_mode` | `aircraft_msgs/msg/ControlSetMode` | Control panel commands: selected mode (`IDLE`, `TAKEOFF`, `CRUISE`, or `SLALOM`), target height, and target speed. Consumed by the trajectory controller. |
| `/flightgear/image` | `sensor_msgs/msg/Image` | Video frames received from FlightGear, published as BGR images. |
