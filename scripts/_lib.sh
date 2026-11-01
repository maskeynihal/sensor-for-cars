# Shared helpers for scripts/*.sh. Source, don't run.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$HOME/bin:$HOME/.wokwi/bin:$PATH"

# Load .env (gitignored secrets, e.g. WOKWI_CLI_TOKEN). Shell env wins.
if [[ -f "$ROOT/.env" ]]; then
  while IFS='=' read -r key val; do
    [[ "$key" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || continue
    val="${val%\"}"; val="${val#\"}"; val="${val%\'}"; val="${val#\'}"
    [[ -z "${!key:-}" && -n "$val" ]] && export "$key=$val"
  done < "$ROOT/.env"
fi

die() { echo "error: $*" >&2; exit 1; }
info() { echo "==> $*"; }

# load_project <name>: sets PDIR, FW_DIR, SIM_DIR, NATIVE_DIR, BUILD + project.env vars
load_project() {
  local p="${1:-}"
  [[ -n "$p" ]] || die "missing project name (P=<name>). Projects: $(list_projects | tr '\n' ' ')"
  PDIR="$ROOT/projects/$p"
  [[ -f "$PDIR/project.env" ]] || die "no project '$p' (expected $PDIR/project.env)"
  SIM_SHIMS=""
  # shellcheck disable=SC1091
  source "$PDIR/project.env"
  FW_DIR="$PDIR/firmware/$NAME"
  SIM_DIR="$PDIR/emulator/wokwi/${NAME}_sim"
  WOKWI_DIR="$PDIR/emulator/wokwi"
  NATIVE_DIR="$PDIR/emulator/native"
  BUILD="$ROOT/build/$NAME"
}

list_projects() {
  for d in "$ROOT"/projects/*/; do
    [[ -f "$d/project.env" ]] && basename "$d"
  done
}

sim_lib_flags() {
  local flags=() s
  for s in $SIM_SHIMS; do flags+=(--library "$PDIR/$s"); done
  echo "${flags[@]:-}"
}
