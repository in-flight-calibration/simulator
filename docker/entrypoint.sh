#!/bin/bash
set -e

source "/opt/ros/${ROS_DISTRO}/setup.bash"
source "${HOME}/ws/install/setup.bash"

set -a
source .env
set +a

exec "$@"
