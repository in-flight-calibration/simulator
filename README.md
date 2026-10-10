# Simulator

Links ROS 2 with FlightGear.

## Installation

Choose one of the following setup paths:

- [Install and run with Docker](#with-docker) — uses the provided [Dockerfile](../../Dockerfile) with ROS 2 Rolling and the CUDA toolkit.
- [Install and run natively, without Docker](#without-docker) — install ROS 2 Rolling and the project dependencies directly on the host.

Both paths use the same source workspace and require FlightGear. See [Install FlightGear](#install-flightgear).

## With Docker

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

The host needs the NVIDIA Container Toolkit configured for Docker to use `--gpus all`. If it is not configured, remove `--gpus all`; the container will run without GPU access. For a GUI-free run, also omit the `DISPLAY` environment variable and X11 socket mount.

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

## Without Docker

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

## Install FlightGear

Download the FlightGear AppImage from [flightgear.org](https://www.flightgear.org/) and place it where you want to run it.

For a native installation, make it executable and launch it:

```bash
chmod +x ./FlightGear.AppImage
./FlightGear.AppImage
```

Replace `FlightGear.AppImage` with the downloaded file's actual name.

For Docker, AppImages may need to be extracted before they can run. From the directory containing the AppImage:

```bash
chmod +x ./FlightGear.AppImage
./FlightGear.AppImage --appimage-extract
./squashfs-root/AppRun
```
