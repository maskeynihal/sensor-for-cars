#!/usr/bin/env bash
# Show how the sim copy differs from the uploadable firmware.  drift.sh <project>
# Exit 0 = identical, 1 = differs (expected while you are mid-change).
source "$(dirname "$0")/_lib.sh"
load_project "${1:-}"
fw="$FW_DIR/$NAME.ino"; sim="$SIM_DIR/${NAME}_sim.ino"
if diff -u --label "firmware/$NAME/$NAME.ino" --label "emulator/wokwi/${NAME}_sim/${NAME}_sim.ino" "$fw" "$sim"; then
  info "in sync: firmware == sim copy"
else
  echo; info "sim copy differs from firmware (promote with: make promote P=$NAME)"; exit 1
fi
