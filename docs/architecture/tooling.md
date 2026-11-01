# Tooling internals

| Piece | Where | Notes |
|---|---|---|
| Project config | `projects/<p>/project.env` | `NAME, PURPOSE, BOARD, FQBN, BAUD, SIM_BOARD, SIM_FQBN, SIM_SHIMS` |
| Script helpers | `scripts/_lib.sh` | `load_project` sets `PDIR FW_DIR SIM_DIR WOKWI_DIR NATIVE_DIR BUILD` |
| Build | `scripts/build.sh <p> firmware\|sim\|all` | `arduino-cli compile --libraries ./libraries [--library <shim>]...` → `build/<p>/{firmware,sim}` |
| Native tests | `scripts/test.sh <p> sim\|firmware\|all [filter]` | clang++ C++17, `-include Arduino.h -x c++ <ino>` → `build/<p>/native/test_*` |
| Wokwi | `scripts/sim.sh <p> [scenario\|--interactive]` | builds sim, `wokwi-cli . --scenario ...` in `emulator/wokwi/` |
| Drift | `scripts/drift.sh <p>` | `diff -u` of the whole sketch folder (`<p>_sim.ino` ↔ `<p>.ino`, plus other tabs) |
| Promote | `scripts/promote.sh <p>` | gated folder mirror with rollback (`build/<p>/firmware.bak/`) |
| Upload | `scripts/upload.sh <p> [port]` | auto-detects the port by FQBN match |
| Monitor | `scripts/monitor.sh <p> <port>` | `arduino-cli monitor` at `BAUD` |
| Scaffold | `scripts/new-project.sh` | copies `templates/project`, replaces `__NAME__` etc., maps the sim FQBN to a Wokwi board part |
| Doctor | `scripts/doctor.sh [--install]` | tools, cores needed by all projects, token |

## Native harness design

- `tools/native/mocks/sim.h` holds all fake-world state, and `sim::reset()` clears it.
- `tools/native/hwtest.h` provides `TEST`, `CHECK`, `CHECK_EQ`, `CHECK_CONTAINS`, and `CHECK_NOT_CONTAINS`.
  `tools/native/hwtest_main.cpp` holds `main()`. It forks a child per test, and kills a test with `alarm(5)` if it hangs.
- The mocks implement only what the repo's sketches call. Extend them as needed, and keep their behaviour faithful.

## Agent entry points

| Agent | Reads |
|---|---|
| Claude Code | `CLAUDE.md` (imports `AGENTS.md`) + `.claude/commands/*` |
| Kiro | `.kiro/steering/*.md` (includes `AGENTS.md`) |
| Others (Codex, Cursor, ...) | `AGENTS.md` |
