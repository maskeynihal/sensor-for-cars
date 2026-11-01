#!/usr/bin/env bash
# Serial monitor.  monitor.sh <project> <port>
source "$(dirname "$0")/_lib.sh"
load_project "${1:-}"
[[ -n "${2:-}" ]] || { arduino-cli board list; die "pass PORT=/dev/cu.usbmodemXXXX"; }
arduino-cli monitor -p "$2" -c "baudrate=$BAUD"
