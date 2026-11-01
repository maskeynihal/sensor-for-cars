#!/usr/bin/env bash
# Promote the tested sim copy to the uploadable firmware.  promote.sh <project>
# Gate: native tests on the sim copy must pass. Then copies the .ino, and
# proves firmware still compiles for the real board and passes native tests.
source "$(dirname "$0")/_lib.sh"
load_project "${1:-}"
fw="$FW_DIR/$NAME.ino"; sim="$SIM_DIR/${NAME}_sim.ino"

if diff -q "$fw" "$sim" >/dev/null; then info "already in sync, nothing to promote"; exit 0; fi

"$ROOT/scripts/test.sh" "$NAME" sim
"$ROOT/scripts/build.sh" "$NAME" sim

mkdir -p "$BUILD"; cp "$fw" "$BUILD/$NAME.ino.bak"
cp "$sim" "$fw"
info "copied sim -> firmware (backup: build/$NAME/$NAME.ino.bak)"

if ! "$ROOT/scripts/build.sh" "$NAME" firmware || ! "$ROOT/scripts/test.sh" "$NAME" firmware; then
  cp "$BUILD/$NAME.ino.bak" "$fw"
  die "firmware build/tests failed after promote -- restored previous firmware"
fi
info "promoted. Review 'git diff ${fw#$ROOT/}', then: make upload P=$NAME PORT=..."
