#!/bin/bash

set -e

cd "$(dirname "${BASH_SOURCE[0]}")"

if ! timeout 2 bash -c '</dev/tcp/127.0.0.1/7447' 2>/dev/null; then
    echo "ERROR: Zenoh router not reachable on localhost:7447" >&2
    echo "Start it first: ros2 run rmw_zenoh_cpp rmw_zenohd" >&2
    exit 1
fi

exec docker compose -f docker/compose.yaml up --no-build "$@"
