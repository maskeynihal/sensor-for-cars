#!/usr/bin/env bash
# Check the toolchain.  doctor.sh [--install]
source "$(dirname "$0")/_lib.sh"
install="${1:-}"
ok=1
chk() { if command -v "$1" >/dev/null; then printf "  ok    %-12s %s\n" "$1" "$($2 2>&1 | head -1)"; else printf "  MISS  %s\n" "$1"; ok=0; fi; }

info "tools"
chk arduino-cli "arduino-cli version"
chk wokwi-cli "wokwi-cli --short-version"
chk clang++ "clang++ --version"
chk python3 "python3 --version"

if command -v arduino-cli >/dev/null; then
  info "board cores (needed by projects)"
  needed=$(for p in $(list_projects); do (source "$ROOT/projects/$p/project.env"; echo "${FQBN%:*}"; echo "${SIM_FQBN%:*}"); done | sort -u)
  for core in $needed; do
    if arduino-cli core list | awk '{print $1}' | grep -qx "$core"; then echo "  ok    $core"
    else echo "  MISS  $core"; ok=0
      [[ "$install" == "--install" ]] && { arduino-cli core update-index; arduino-cli core install "$core"; }
    fi
  done
fi

info "wokwi token"
if [[ -n "${WOKWI_CLI_TOKEN:-}" ]]; then echo "  ok    WOKWI_CLI_TOKEN set"; else echo "  MISS  WOKWI_CLI_TOKEN (only needed for make sim) -> https://wokwi.com/dashboard/ci, put it in .env"; fi

if [[ "$install" == "--install" ]]; then
  command -v arduino-cli >/dev/null || brew install arduino-cli
  command -v wokwi-cli >/dev/null || { echo "install wokwi-cli: curl -L https://wokwi.com/ci/install.sh | sh"; }
fi
[[ $ok == 1 ]] && info "all good" || { info "missing items above (make doctor-install)"; exit 1; }
