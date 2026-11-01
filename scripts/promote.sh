#!/usr/bin/env bash
# Promote the tested sim copy to the uploadable firmware.  promote.sh <project>
# Gate: native tests on the sim copy must pass. Then mirrors the sim sketch
# folder into firmware (<p>_sim.ino -> <p>.ino, other tabs as-is), and
# proves firmware still compiles for the real board and passes native tests.
source "$(dirname "$0")/_lib.sh"
load_project "${1:-}"
fw="$FW_DIR/$NAME.ino"; sim="$SIM_DIR/${NAME}_sim.ino"

if "$ROOT/scripts/drift.sh" "$NAME" >/dev/null; then info "already in sync, nothing to promote"; exit 0; fi

"$ROOT/scripts/test.sh" "$NAME" sim
"$ROOT/scripts/build.sh" "$NAME" sim

bak="$BUILD/firmware.bak"
rm -rf "$bak"; mkdir -p "$BUILD"; cp -R "$FW_DIR" "$bak"
rsync -a --delete --exclude "${NAME}_sim.ino" --exclude "$NAME.ino" --exclude .DS_Store "$SIM_DIR/" "$FW_DIR/"
cp "$sim" "$fw"
info "mirrored sim -> firmware (backup: build/$NAME/firmware.bak/)"

if ! "$ROOT/scripts/build.sh" "$NAME" firmware || ! "$ROOT/scripts/test.sh" "$NAME" firmware; then
  rsync -a --delete "$bak/" "$FW_DIR/"
  die "firmware build/tests failed after promote -- restored previous firmware"
fi
info "promoted. Review 'git diff ${fw#$ROOT/}', then: make upload P=$NAME PORT=..."
