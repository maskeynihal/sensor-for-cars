#!/usr/bin/env bash
# Native (host) tests.  test.sh <project> [sim|firmware|all] [test-name-filter]
# Compiles the UNMODIFIED .ino against tools/native/mocks and runs
# emulator/native/test_*.cpp. "all" runs the suite against both copies.
source "$(dirname "$0")/_lib.sh"
load_project "${1:-}"
which="${2:-all}"
filter="${3:-}"

CXX="${CXX:-clang++}"
MOCKS="$ROOT/tools/native/mocks"
tests=("$NATIVE_DIR"/test_*.cpp)
[[ -e "${tests[0]}" ]] || die "no tests in $NATIVE_DIR (expected test_*.cpp)"

run_suite() {
  local label="$1" ino="$2" out="$BUILD/native/test_$1"
  [[ -f "$ino" ]] || die "missing $ino"
  mkdir -p "$BUILD/native"
  info "native tests: $label (${ino#$ROOT/})"
  # extra .cpp/.c tabs in the sketch folder are compiled too
  local sketch_dir extra=()
  sketch_dir="$(dirname "$ino")"
  while IFS= read -r f; do extra+=("$f"); done < <(find "$sketch_dir" -maxdepth 1 \( -name '*.cpp' -o -name '*.c' \) | sort)
  "$CXX" -std=c++17 -g -O0 -Wall -Wno-unused-variable \
    -I "$MOCKS" -I "$ROOT/tools/native" -I "$NATIVE_DIR" -I "$sketch_dir" \
    "$MOCKS/arduino_mock.cpp" "$ROOT/tools/native/hwtest_main.cpp" "${tests[@]}" ${extra[@]+"${extra[@]}"} \
    -x c++ -include Arduino.h "$ino" \
    -o "$out"
  "$out" ${filter:+"$filter"}
}

rc=0
case "$which" in
  sim) run_suite sim "$SIM_DIR/${NAME}_sim.ino" ;;
  firmware) run_suite firmware "$FW_DIR/$NAME.ino" ;;
  all)
    run_suite sim "$SIM_DIR/${NAME}_sim.ino" || rc=1
    run_suite firmware "$FW_DIR/$NAME.ino" || rc=1 ;;
  *) die "unknown target '$which' (sim|firmware|all)" ;;
esac
exit $rc
