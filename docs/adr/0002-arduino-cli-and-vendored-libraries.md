# 0002. arduino-cli with vendored libraries

- Status: Accepted
- Date: 2026-11-01
- Projects: all

## Context

Sketches were built in the Arduino IDE. Agents need a headless, scriptable build. Library versions drift
when they're installed by name on each machine.

## Options

1. **Arduino IDE**: GUI only, an agent can't drive it.
2. **PlatformIO**: strong, but it needs a different project layout (`src/main.cpp`, `platformio.ini`), and the
   existing sketches and docs assume `.ino` + Arduino cores.
3. **arduino-cli + `--libraries ./libraries`**: same cores and `.ino` format as the IDE, scriptable, and libraries
   pinned in the repo.

## Decision

Option 3. All builds go through `scripts/build.sh`, which calls
`arduino-cli compile --fqbn <FQBN> --libraries ./libraries`. Libraries in `libraries/` are third-party copies and
read-only.

## Consequences

- Same result in the IDE and the CLI. A fresh clone builds without installing any library.
- Adding or upgrading a library is a deliberate commit (see `docs/howto/add-a-library.md`).
- The repo is larger (vendored source).
