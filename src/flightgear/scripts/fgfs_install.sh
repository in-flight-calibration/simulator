#!/bin/bash
set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="/usr/share/games/flightgear"

sudo apt update
sudo apt install -y flightgear

sudo install -d "$FG_ROOT"
sudo cp -a "$SCRIPT_DIR/fgfs/." "$FG_ROOT/"

echo "Configuration copied successfully"
