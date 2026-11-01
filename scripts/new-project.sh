#!/usr/bin/env bash
# Scaffold a project from templates/project.
#   new-project.sh <name> <fqbn> <sim_fqbn> [board label] [purpose] [baud]
# Example:
#   new-project.sh tyre_pressure esp32:esp32:esp32 esp32:esp32:esp32 "ESP32 DevKit" "TPMS reader" 115200
# The agent asks the user for these first (see .claude/commands/new-project.md).
source "$(dirname "$0")/_lib.sh"

name="${1:-}"; fqbn="${2:-}"; sim_fqbn="${3:-}"
board="${4:-$fqbn}"; purpose="${5:-TODO: one-line purpose}"; baud="${6:-115200}"

[[ "$name" =~ ^[a-z][a-z0-9_]*$ ]] || die "name must be snake_case: [a-z][a-z0-9_]*"
[[ -n "$fqbn" && -n "$sim_fqbn" ]] || die "usage: new-project.sh <name> <fqbn> <sim_fqbn> [board] [purpose] [baud]"
dest="$ROOT/projects/$name"
[[ ! -e "$dest" ]] || die "projects/$name already exists"

# Wokwi board part + firmware artifact for the sim FQBN.
case "$sim_fqbn" in
  arduino:avr:uno)        part=wokwi-arduino-uno;  ext=hex; sim_board="Arduino Uno (ATmega328P)" ;;
  arduino:avr:nano)       part=wokwi-arduino-nano; ext=hex; sim_board="Arduino Nano (ATmega328P)" ;;
  arduino:avr:mega)       part=wokwi-arduino-mega; ext=hex; sim_board="Arduino Mega 2560" ;;
  esp32:esp32:esp32s3*)   part=board-esp32-s3-devkitc-1; ext=merged.bin; sim_board="ESP32-S3 DevKitC-1" ;;
  esp32:esp32:esp32c3*)   part=board-esp32-c3-devkitm-1; ext=merged.bin; sim_board="ESP32-C3 DevKitM-1" ;;
  esp32:esp32:esp32)      part=board-esp32-devkit-c-v4; ext=merged.bin; sim_board="ESP32 DevKit C v4" ;;
  rp2040:rp2040:*|arduino:mbed_rp2040:*) part=wokwi-pi-pico; ext=uf2; sim_board="Raspberry Pi Pico" ;;
  *) die "no Wokwi mapping for '$sim_fqbn'. Supported sims: Uno/Nano/Mega, ESP32(-S3/-C3), Pico. Add a case here." ;;
esac

info "scaffold projects/$name ($board -> sim $sim_board)"
cp -R "$ROOT/templates/project" "$dest"
# rename __NAME__ in paths (deepest first)
find "$dest" -depth -name '*__NAME__*' | while read -r p; do
  mv "$p" "$(dirname "$p")/$(basename "$p" | sed "s/__NAME__/$name/g")"
done
# ESP32 boards need their UART wired to Wokwi's serial monitor; AVR/Pico do it implicitly.
serial_conn=""
[[ "$ext" == "merged.bin" ]] && serial_conn='[ "mcu:TX", "$serialMonitor:RX", "", [] ], [ "mcu:RX", "$serialMonitor:TX", "", [] ]'

esc() { printf '%s' "$1" | sed 's/[&|\\]/\\&/g'; }
find "$dest" -type f | while read -r f; do
  sed -i '' \
    -e "s|__NAME__|$name|g" -e "s|__FQBN__|$(esc "$fqbn")|g" -e "s|__SIM_FQBN__|$(esc "$sim_fqbn")|g" \
    -e "s|__BOARD__|$(esc "$board")|g" -e "s|__SIM_BOARD__|$(esc "$sim_board")|g" \
    -e "s|__PURPOSE__|$(esc "$purpose")|g" -e "s|__BAUD__|$baud|g" \
    -e "s|__WOKWI_BOARD__|$part|g" -e "s|__FW_EXT__|$ext|g" -e "s|__SERIAL_CONN__|$(esc "$serial_conn")|g" -e "s|__DATE__|$(date +%Y-%m-%d)|g" "$f"
done

# make sure the cores exist
for core in "${fqbn%:*}" "${sim_fqbn%:*}"; do
  arduino-cli core list | awk '{print $1}' | grep -qx "$core" || {
    info "installing core $core"; arduino-cli core install "$core" || echo "  (core install failed -- may need: arduino-cli config add board_manager.additional_urls <url>)"; }
done

info "verify: native tests + both builds"
"$ROOT/scripts/test.sh" "$name" all
"$ROOT/scripts/build.sh" "$name" all || echo "  build failed -- fix before continuing"
info "created projects/$name. Next: fill docs/, write an idea in docs/ideas/, ADR if a new board/part."
