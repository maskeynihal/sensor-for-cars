#!/usr/bin/env bash
# Wokwi simulation of the sim copy.  sim.sh <project> [scenario.yaml|--interactive]
#   no 2nd arg  -> run every emulator/wokwi/scenarios/*.yaml
#   scenario    -> run just that one (path relative to emulator/wokwi/)
#   --interactive -> free-run 60 s with serial passthrough + screenshot
# Needs WOKWI_CLI_TOKEN (https://wokwi.com/dashboard/ci). Free tier: 50 sim-min / 30 days.
source "$(dirname "$0")/_lib.sh"
load_project "${1:-}"
arg="${2:-}"

command -v wokwi-cli >/dev/null || die "wokwi-cli not installed (make doctor)"
[[ -n "${WOKWI_CLI_TOKEN:-}" ]] || die "WOKWI_CLI_TOKEN not set. Get one at https://wokwi.com/dashboard/ci and put it in .env (see .env.example)"

"$ROOT/scripts/build.sh" "$NAME" sim
mkdir -p "$BUILD/wokwi"
cd "$WOKWI_DIR"

if [[ "$arg" == "--interactive" ]]; then
  wokwi-cli . --timeout 60000 --timeout-exit-code 0 --interactive \
    --serial-log-file "$BUILD/wokwi/serial.log" \
    --screenshot-part oled --screenshot-time 59000 --screenshot-file "$BUILD/wokwi/oled.png"
  exit 0
fi

if [[ -n "$arg" ]]; then scenarios=("$arg"); else scenarios=(scenarios/*.yaml); fi
[[ -e "${scenarios[0]}" ]] || die "no scenarios in $WOKWI_DIR/scenarios"

rc=0
for s in "${scenarios[@]}"; do
  base="$(basename "$s" .yaml)"
  info "wokwi scenario: $s"
  if wokwi-cli . --scenario "$s" --timeout 30000 \
       --serial-log-file "$BUILD/wokwi/$base.serial.log" \
       --screenshot-part oled --screenshot-time 8000 --screenshot-file "$BUILD/wokwi/$base.oled.png"; then
    echo "  PASS  $base"
  else
    echo "  FAIL  $base  (serial log: build/$NAME/wokwi/$base.serial.log)"; rc=1
  fi
done
exit $rc
