# syntax=docker/dockerfile:1
ARG ROS_DISTRO=lyrical
FROM ros:${ROS_DISTRO}-ros-base

ARG DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# VirtualGL apt repository
RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates gnupg wget \
    && wget -q -O- https://packagecloud.io/dcommander/virtualgl/gpgkey \
        | gpg --dearmor -o /etc/apt/trusted.gpg.d/VirtualGL.gpg \
    && wget -q -O /etc/apt/sources.list.d/VirtualGL.list \
        https://raw.githubusercontent.com/VirtualGL/repo/main/VirtualGL.list \
    && rm -rf /var/lib/apt/lists/*

# Build toolchain and libraries for PX4 and the ROS workspace
RUN apt-get update \
    && apt-get install -y \
        build-essential ccache cmake file git make ninja-build rsync unzip zip \
        libssl-dev libxml2-dev libxml2-utils \
        python3-dev python3-pip python3-setuptools python3-wheel python3-yaml \
        libopencv-dev \
    && rm -rf /var/lib/apt/lists/*

# ROS packages used by the workspace
RUN apt-get update \
    && apt-get install -y \
        ros-${ROS_DISTRO}-cv-bridge \
        ros-${ROS_DISTRO}-compressed-image-transport \
        ros-${ROS_DISTRO}-geographic-msgs \
        ros-${ROS_DISTRO}-image-transport \
        ros-${ROS_DISTRO}-rmw-zenoh-cpp \
        ros-${ROS_DISTRO}-rosidl-default-generators \
        ros-${ROS_DISTRO}-tf2-eigen \
        ros-${ROS_DISTRO}-tf2-geometry-msgs \
        ros-${ROS_DISTRO}-tf2-ros \
    && rm -rf /var/lib/apt/lists/*

# FlightGear and headless GPU rendering
RUN apt-get update \
    && apt-get install -y \
        flightgear xvfb x11-utils mesa-utils libegl1 virtualgl \
    && rm -rf /var/lib/apt/lists/*

# GStreamer video streaming
RUN apt-get update \
    && apt-get install -y \
        gstreamer1.0-tools \
        gstreamer1.0-plugins-base \
        gstreamer1.0-plugins-good \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-plugins-ugly \
        gstreamer1.0-libav \
    && rm -rf /var/lib/apt/lists/*

# Runtime utilities
RUN apt-get update \
    && apt-get install -y procps tini \
    && rm -rf /var/lib/apt/lists/*

# Simulator user and X11 socket directory
RUN install -d -m 1777 /tmp/.X11-unix \
    && useradd --create-home --shell /bin/bash sim \
    && install -d -o sim -g sim /home/sim/.fgfs

COPY src/flightgear/scripts/fgfs/ /usr/share/games/flightgear/
COPY --chmod=755 docker/entrypoint.sh /entrypoint.sh

USER sim
WORKDIR /home/sim/ws

# /usr/games holds fgfs; no D-Bus bus in the container, so keep libdbus from aborting fgfs
ENV PATH=/home/sim/.local/bin:${PATH}:/usr/games \
    DBUS_FATAL_WARNINGS=0 \
    PX4_DIR=/home/sim/ws/src/px4/PX4-Autopilot \
    DDS_TOPICS_FILE=ifc_dds_topics.yaml \
    GIT_SUBMODULES_ARE_EVIL=1 \
    CCACHE_DISABLE=1 \
    RMW_IMPLEMENTATION=rmw_zenoh_cpp

# Same per-user install as PX4's ubuntu.sh, leaving the ROS Python packages untouched
COPY --chown=sim:sim src/px4/PX4-Autopilot/Tools/setup/requirements.txt /tmp/px4-requirements.txt
RUN pip3 install --user --break-system-packages --no-cache-dir -r /tmp/px4-requirements.txt

# PX4 needs its git metadata to generate the version header
COPY --chown=sim:sim .git/modules/src/px4 .git/modules/src/px4
COPY --chown=sim:sim src/px4/PX4-Autopilot src/px4/PX4-Autopilot
RUN make -C "$PX4_DIR" px4_sitl_zenoh "CMAKE_ARGS=-DDDS_TOPICS_FILE=${DDS_TOPICS_FILE}"

COPY --chown=sim:sim src/px4/CMakeLists.txt src/px4/package.xml src/px4/
COPY --chown=sim:sim src/px4/launch src/px4/launch
COPY --chown=sim:sim src/px4_msgs src/px4_msgs
COPY --chown=sim:sim src/ifc_msgs src/ifc_msgs
COPY --chown=sim:sim src/flightgear src/flightgear
COPY --chown=sim:sim src/simulator src/simulator
RUN source "/opt/ros/${ROS_DISTRO}/setup.bash" && colcon build --base-paths src

COPY --chown=sim:sim config.yaml /home/sim/config.yaml

ENTRYPOINT ["/usr/bin/tini", "-g", "--", "/entrypoint.sh"]
CMD ["ros2", "launch", "simulator", "launch.py", "config:=/home/sim/config.yaml", "zenoh_router:=false"]
