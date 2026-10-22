# Workspace Guidance

## Project Map

- The top-level launch flow is `src/simulator/launch/launch.py`: it starts the Zenoh router (skipped with `zenoh_router:=false`), includes the PX4 launch file, and starts FlightGear.
- `src/px4/launch/px4.launch.py` launches PX4 SITL with `make px4_sitl_zenoh sihsim_ifc`; PX4 behavior and build conventions belong to the nested `src/px4/PX4-Autopilot/` checkout.
- `src/flightgear/` owns the ROS 2 FlightGear bridge. Its control and video nodes are in `src/flightgear/src/`; `src/flightgear/scripts/fgfs.sh` starts FlightGear and its GStreamer video stream. Headless Xvfb is the default; VirtualGL is used when installed, otherwise rendering falls back to software.
- `src/ifc_msgs/` and `src/px4_msgs/` contain ROS message packages. Keep interface changes compatible with their consumers, especially `flightgear` and the PX4 build.
- The workspace also includes PlotJuggler packages; do not assume they are part of the simulator launch path.

## Build And Run

- Read [README.md](README.md) for installation prerequisites and the documented setup/run entry points.
- `./simulator.sh` sources the newest version-sorted ROS installation under `/opt/ros`, loads `.env`, builds the whole colcon workspace, and launches the simulator with `config.yaml`. Run it from the workspace root; it is an integration run, not a build-only command.
- The configured `Build px4_msgs` task uses `/opt/ros/lyrical` and runs `colcon build --packages-select px4_msgs --cmake-force-configure`. Source the intended ROS installation and workspace setup before using `colcon` directly.
- The PX4 launch requires `PX4_DIR` to point to a valid PX4 checkout. It reads `DDS_TOPICS_FILE` (default `ifc_dds_topics.yaml`). `simulator.sh` sources `.env`; do not assume these variables are set in a fresh shell.
- There are no known dedicated automated tests for the workspace-owned simulator and FlightGear packages. Prefer a focused `colcon build --packages-select <package>` for code changes, followed by a launch or targeted runtime smoke check when the required simulator dependencies are available.
- Docker: all files are in `docker/` (`Dockerfile`, `Dockerfile.dockerignore`, `compose.yaml`; build context is the workspace root). `docker/build.sh` builds the image; `./simulator_docker.sh` exits if no Zenoh router listens on `localhost:7447`, otherwise starts Compose without building. See the Docker section of [README.md](README.md).
- The container uses host networking and launches with `zenoh_router:=false`, so PX4 and ROS nodes use the host router on `localhost:7447`. It mounts `config.yaml` read-only, caches FlightGear TerraSync data in the `fgfs` volume, and does not read `.env`. FlightGear always renders on the dedicated NVIDIA GPU via VirtualGL EGL (Compose GPU reservation; host needs nvidia-container-toolkit).
- Docker changes may touch native files, but the native run must keep working as before: `./simulator.sh` and default launch behavior must stay functionally unchanged.

## Change Guidance

- Keep ROS launch orchestration in `src/simulator/`; keep FlightGear process, control, and video behavior in `src/flightgear/`; keep message schema changes in the relevant message package.
- For video-stream issues, distinguish the GStreamer sender from the ROS receiver. Check `/tmp/fgfs.log`, `/tmp/gst.log`, and the FlightGear node output, then validate capture and publish rates; `src/flightgear/scripts/fgfs_videotest.sh` is a manual stream smoke test.
- Prefer the smallest implementation consistent with the existing code. Avoid adding state, timing abstractions, or dependencies unless the behavior requires them.
- `setup/ubuntu.sh` installs system dependencies and may make machine-level changes; inspect the intended changes before running it.
- `.env` is local runtime configuration. Do not expose its values in chat or overwrite it as part of unrelated work.
- Preserve existing user changes, including untracked message-package files. Do not clean or regenerate `build/`, `install/`, or `log/` unless the task requires it.
- When editing `src/px4/PX4-Autopilot/`, follow its nested [AGENTS.md](src/px4/PX4-Autopilot/AGENTS.md) and applicable PX4 instructions instead of duplicating them here.
