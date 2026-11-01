#!/usr/bin/env bash
# Show how the sim copy differs from the uploadable firmware.  drift.sh <project>
# Compares the whole sketch folder: <p>_sim.ino <-> <p>.ino, plus every other tab (.h/.cpp/...).
# Exit 0 = identical, 1 = differs (expected while you are mid-change).
source "$(dirname "$0")/_lib.sh"
load_project "${1:-}"
rc=0
diff -u --label "firmware/$NAME/$NAME.ino" --label "emulator/wokwi/${NAME}_sim/${NAME}_sim.ino" \
  "$FW_DIR/$NAME.ino" "$SIM_DIR/${NAME}_sim.ino" || rc=1
others=$( { (cd "$FW_DIR" && ls -1A); (cd "$SIM_DIR" && ls -1A); } | { grep -vxE "$NAME\.ino|${NAME}_sim\.ino|\.DS_Store" || true; } | sort -u)
for f in $others; do
  if [[ ! -e "$FW_DIR/$f" ]]; then echo "only in sim copy: $f"; rc=1
  elif [[ ! -e "$SIM_DIR/$f" ]]; then echo "only in firmware: $f"; rc=1
  else diff -ru --label "firmware/$NAME/$f" --label "emulator/wokwi/${NAME}_sim/$f" "$FW_DIR/$f" "$SIM_DIR/$f" || rc=1
  fi
done
if [[ $rc == 0 ]]; then info "in sync: firmware == sim copy"
else echo; info "sim copy differs from firmware (promote with: make promote P=$NAME)"; fi
exit $rc
