#!/usr/bin/env bash
# Compile a project.  build.sh <project> [firmware|sim|all]
source "$(dirname "$0")/_lib.sh"
load_project "${1:-}"
what="${2:-all}"

build_fw() {
  info "firmware: $NAME for $BOARD ($FQBN)"
  arduino-cli compile --fqbn "$FQBN" --libraries "$ROOT/libraries" \
    --build-path "$BUILD/firmware" "$FW_DIR"
}

build_sim() {
  [[ -d "$SIM_DIR" ]] || die "no sim sketch at $SIM_DIR"
  info "sim: ${NAME}_sim for $SIM_BOARD ($SIM_FQBN)"
  # shellcheck disable=SC2046
  arduino-cli compile --fqbn "$SIM_FQBN" --libraries "$ROOT/libraries" $(sim_lib_flags) \
    --build-path "$BUILD/sim" "$SIM_DIR"
}

case "$what" in
  firmware) build_fw ;;
  sim) build_sim ;;
  all) build_sim; build_fw ;;
  *) die "unknown target '$what' (firmware|sim|all)" ;;
esac
