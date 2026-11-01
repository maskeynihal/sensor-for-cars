# 0003. `projects/<name>/` layout with separate firmware and emulator copies

- Status: Accepted
- Date: 2026-11-01
- Projects: all

## Context

The repo started as a flat `sketches/` folder with a single product. More car projects are planned, possibly on
different processors. The user requires:
- code in the "sketches" folder is always uploadable;
- code written and tested locally goes through an emulator and lives in a different folder.

arduino-cli sketches can't `#include` files outside their own folder, so firmware and emulator can't share a
source file.

## Options

1. Keep `sketches/` and add a parallel `emulator/<name>/`. Docs are scattered and each project spans three trees.
2. **`projects/<name>/{firmware,emulator,docs}`** with a `project.env` per project.

## Decision

Option 2.
- `firmware/<name>/<name>.ino` is the uploadable sketch.
- `emulator/wokwi/<name>_sim/<name>_sim.ino` is the working copy.
- Once promoted, the two `.ino` files are **byte-identical**. Everything sim-specific lives outside the .ino: shim
  libraries, diagram.json, scenarios, native tests.
- `make drift` shows the difference. `make promote` copies sim → firmware, gated by tests and a real-board compile.

Helper sketches move to `projects/_utilities/`. Superseded ones move to `projects/_archive/`.

## Consequences

- Two copies can drift. `make drift` and the promote gate keep that visible.
- Native tests compile *both* copies, so a hand edit to firmware still gets tested.
- Every script takes `P=<name>`.
