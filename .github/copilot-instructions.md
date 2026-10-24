# Copilot Instructions

ROS 2 (Lyrical) colcon workspace: PX4 SITL + FlightGear camera simulation, bridged over Zenoh. See [README.md](README.md) and [AGENTS.md](AGENTS.md).

## Build and run

- `./simulator.sh` (from workspace root): sources the newest `/opt/ros/*`, loads `.env` (provides `PX4_DIR`, `PX4_CMD`, `PX4_TARGET`), runs `colcon build` for the whole workspace, then `ros2 launch simulator launch.py config:=config.yaml`. It is an integration run, not build-only.
- Focused build: source `/opt/ros/lyrical/setup.bash` (and `install/setup.bash`), then `colcon build --packages-select <package>`. For `px4_msgs`, add `--cmake-force-configure`.
- Docker: `./docker/build.sh`, start a host router (`rmw_zenohd` on `localhost:7447`), then `./simulator_docker.sh`. Don't run native and Docker simultaneously (shared host network/ports).
- No automated tests or linters exist for the workspace-owned packages; validate with a package build plus a launch/runtime smoke check. `src/flightgear/scripts/fgfs_videotest.sh` smoke-tests the video stream.
- Docker host router: `export RMW_IMPLEMENTATION=rmw_zenoh_cpp; ZENOH_CONFIG_OVERRIDE='listen/endpoints=["tcp/127.0.0.1:7447"]' ros2 run rmw_zenoh_cpp rmw_zenohd`. Host and image must share ROS distro, `rmw_zenoh_cpp`, message definitions, and `ROS_DOMAIN_ID` (PX4 `ZENOH_DOMAIN_ID` too). Apply `config.yaml` edits with `docker compose -f docker/compose.yaml restart`; rebuild the image after source changes; init submodules first (`git submodule update --init --recursive`).
- `setup/ubuntu.sh` makes machine-level changes; inspect before running.

## Architecture

- `src/simulator/launch/launch.py` is the top-level launch: Zenoh router (skip with `zenoh_router:=false`), PX4 launch, FlightGear. `config.yaml` is a ROS 2 parameter file (everything under `/**: ros__parameters`, grouped as `camera.*`, `fg.*`, `gst.*`) passed via `--ros-args --params-file`; nodes declare every parameter themselves (no yaml-cpp). Use `rclcpp::create_timer(this, get_clock(), rclcpp::Duration, cb)`, not `create_wall_timer`/`std::chrono`.
- `src/px4/launch/px4.launch.py` runs PX4 SITL via `make px4_sitl_zenoh sihsim_ifc` in the nested `src/px4/PX4-Autopilot/` submodule (follow its own `AGENTS.md` when editing there).
- `src/px4_msgs/` is generated from uORB topics (via `-DDDS_TOPICS_FILE` in `PX4_CMD`, default `ifc_dds_topics.yaml`); `src/ifc_msgs/` holds custom interfaces. Both are consumed by `flightgear` and the PX4 build, so keep changes compatible.
- `src/flightgear/` is the C++ ROS 2 bridge: three nodes in one executable: `flightgear_control_node` (pose -> FlightGear UDP), `flightgear_video_node` (GStreamer stream -> image topic), `flightgear_process_node` (starts `fgfs.sh` via `process.cpp`; owns the `Camera` params and passes it by const& to the other nodes; `camera.hpp` stays ROS-free). `scripts/fgfs.sh` starts FlightGear (headless Xvfb; VirtualGL if installed, else software rendering) plus a GStreamer video sender.
- PlotJuggler packages are in the workspace but not part of the launch path.

## Conventions

- Keep launch orchestration in `src/simulator/`, FlightGear process/control/video in `src/flightgear/`, schema changes in the relevant msgs package.
- Native `./simulator.sh` behavior must stay unchanged when changing Docker files (all in `docker/`; the container launches with `zenoh_router:=false`, mounts `config.yaml` read-only, uses the same `.env` copied into the image (sourced at build and by `docker/entrypoint.sh`; rebuild after changing it), and requires an NVIDIA GPU).
- Video issues: separate the GStreamer sender from the ROS receiver; check `/tmp/fgfs.log`, `/tmp/gst.log`, node output, then capture/publish rates.
- Don't expose or overwrite `.env`; don't clean/regenerate `build/`, `install/`, `log/`; preserve untracked message-package files.
- Prefer the smallest change consistent with existing code; avoid new state, timing abstractions, or dependencies unless required.
