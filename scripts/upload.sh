#!/usr/bin/env bash
# Compile + upload firmware to a real board.  upload.sh <project> [port]
# Port auto-detected when exactly one matching board is connected.
source "$(dirname "$0")/_lib.sh"
load_project "${1:-}"
port="${2:-}"

if [[ -z "$port" ]]; then
  ports=$(FQBN="$FQBN" arduino-cli board list --format json | FQBN="$FQBN" python3 -c '
import json,sys,os
fqbn=os.environ["FQBN"]
d=json.load(sys.stdin)
for p in d.get("detected_ports",[]):
    for b in p.get("matching_boards",[]) or []:
        if b.get("fqbn")==fqbn: print(p["port"]["address"])')
  n=$(printf "%s" "$ports" | grep -c . || true)
  [[ "$n" == "1" ]] || { arduino-cli board list; die "found $n '$BOARD' boards; pass PORT=/dev/cu.usbmodemXXXX"; }
  port="$ports"
fi

"$ROOT/scripts/build.sh" "$NAME" firmware
info "upload $NAME -> $port ($FQBN)"
arduino-cli upload --fqbn "$FQBN" --input-dir "$BUILD/firmware" -p "$port" "$FW_DIR"
info "done. Watch serial: make monitor P=$NAME PORT=$port"
