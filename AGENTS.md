# AGENTS.md

Instructions for any coding agent (Claude Code, Kiro, Codex, Cursor, ...) working in this repo.
Humans: read [README.md](README.md) first.

This repo holds microcontroller projects (sensors, displays, alarms, all for cars). Each project has
**uploadable firmware** and an **emulator copy**. The agent changes and tests the emulator
copy on this Mac. Code reaches the real board only through a gated promote step.

## Hard rules

1. **`projects/<p>/firmware/<p>/` must always be uploadable.** It must compile for the real board
   (`FQBN` in `project.env`) using only the root `libraries/` folder. No mocks, no sim shims, no test hooks, no
   `#ifdef SIM`. Change it only with `make promote P=<p>`. Edit it by hand only when the change can't be simulated
   (see [promote how-to](docs/howto/promote-sim-to-firmware.md)), and say so.
2. **Make and test changes in the emulator copy first:** `projects/<p>/emulator/wokwi/<p>_sim/<p>_sim.ino`.
   Once promoted, it is byte-identical to the firmware. Sim-only differences live *outside* the .ino, in
   `emulator/wokwi/shims/` and `diagram.json`.
3. **Never edit `libraries/`.** These are vendored third-party libraries. When a project needs a new one, follow
   [add-a-library](docs/howto/add-a-library.md).
4. **A green test is required before you claim something works.** Run `make test P=<p>` (native, always available).
   Also run `make sim P=<p>` when `WOKWI_CLI_TOKEN` is set. If you couldn't run it, say so. You can't verify upload or
   real-hardware behaviour from here. Hand those steps to the user with exact commands.
5. **Don't upload unless the user asks.** Flashing a board is a physical, user-visible action.
6. **New project: ask first, then scaffold.** See [Starting a new project](#starting-a-new-project).
7. **Record decisions.** For a new board, part, library, or architecture choice, write an ADR in `docs/adr/`. Ideas
   that aren't committed to yet go in `docs/ideas/`.

## Layout

```
projects/<p>/
  project.env                 board FQBN, sim FQBN, baud, sim shims   (read by scripts/)
  README.md                   what it is, status, commands
  firmware/<p>/<p>.ino        UPLOADABLE sketch (real board)
  emulator/
    wokwi/<p>_sim/<p>_sim.ino working copy, built for the Wokwi board
    wokwi/shims/              sim-only stand-in libraries (e.g. SH1106 -> SSD1306)
    wokwi/diagram.json        simulated circuit
    wokwi/scenarios/*.yaml    Wokwi automation tests
    native/test_*.cpp         host tests, compiled against the real .ino + tools/native/mocks
  docs/                       wiring, BOM, architecture for this project
projects/_utilities/          small helper sketches (blink, I2C scanner)
projects/_archive/            superseded sketches, kept for reference
libraries/                    vendored Arduino libraries (read-only)
tools/native/                 host-side Arduino mocks + hwtest runner (shared by all projects)
templates/project/            skeleton used by `make new`
scripts/                      build / test / sim / promote / upload (Makefile wraps these)
docs/ideas/  docs/adr/  docs/howto/  docs/architecture/
build/                        all outputs (gitignored)
```

## The loop

```
edit sim copy -> make test P=<p> -> make sim P=<p> -> make promote P=<p> -> (user) make upload P=<p>
```

| Step | Command | Proves |
|---|---|---|
| Host tests | `make test P=<p>` (`T=<name>` filters) | Logic, thresholds, timing, serial/OLED text, sensor-failure paths. Runs on both copies. |
| Sim build | `make build-sim P=<p>` | Fits on the sim board (watch the RAM % on AVR) |
| Wokwi | `make sim P=<p>` / `S=scenarios/x.yaml` | Real libraries + simulated parts on a real MCU core |
| Drift | `make drift P=<p>` | What still has to be promoted |
| Promote | `make promote P=<p>` | Copies the sim copy to firmware, then compiles for the real board and runs the tests again. Rolls back on failure. |
| Upload | `make upload P=<p> PORT=/dev/cu.usbmodemXXXX` | User only |

Full diagram: [docs/architecture/dev-workflow.md](docs/architecture/dev-workflow.md).

## Writing sketch code that stays testable

- Use non-blocking timing (`millis()` deltas). The native harness fakes time: `delay()` advances the clock.
- Define functions before use, or add prototypes. The native build doesn't auto-generate them like the Arduino IDE.
- Keep thresholds and pins as `#define`/`const` at the top, with a header comment block listing board, pins, and
  serial log format (see `water_monitor.ino`).
- Log state changes to Serial in a stable format. Both test tiers assert on it.
- If a sketch uses a library with no mock in `tools/native/mocks/`, add a minimal mock that behaves like the real
  part (see `DallasTemperature.h`: it latches the reading on `requestTemperatures()`). Expose its inputs in `sim.h`.
- Watch RAM on AVR sim boards (2 KB Uno). Use `F("...")` for constant strings. If the sim board can't fit the
  sketch, move the sim to a bigger Wokwi board (Mega) and write an ADR.

## Writing tests

- Native: `projects/<p>/emulator/native/test_<p>.cpp`, using `TEST`/`CHECK*` from `tools/native/hwtest.h`. Each test runs in
  a fresh forked process. Set inputs via `sim::` (`ds18b20C`, `dhtTempC`, `i2cDevices`, ...). Then call `setup()` and
  `sim::runFor(ms)`, and assert on `sim::pinLevel`, `sim::togglesBetween`, `sim::serialOut`, and `sim::lastFrameText`.
- Wokwi: `emulator/wokwi/scenarios/<behaviour>.yaml`, using `wait-serial`, `set-control`, `expect-pin`, and `delay`.
  Free tier: 50 sim-minutes per 30 days, so keep scenarios short.
- Test boundaries (exactly at each threshold), failure paths (sensor unplugged, NaN), and recovery.
- Mutation check: after you write a test, break the code briefly and confirm the test fails.

## Starting a new project

Before you create anything, ask the user (use your question tool if you have one):

1. **Name**: snake_case.
2. **What it does**: one sentence, plus inputs (sensors) and outputs (display, buzzer, relay, radio).
3. **Processor/board**: e.g. UNO R4 WiFi, ESP32 DevKit, ESP32-S3, Pico, Nano. Get the FQBN with `arduino-cli board listall <word>`.
4. **Sim board**: the closest board Wokwi supports: Uno/Nano/Mega (AVR), ESP32/-S3/-C3, Pico. Same as the real board when possible.
5. **Parts**: each sensor/display, so you can check the Wokwi part exists ([docs](https://docs.wokwi.com/getting-started/supported-hardware)). If a part is missing, plan a shim and record it in an ADR.
6. **Power and environment**: 12 V car supply? Battery? Under the hood? This affects regulators and parts.

Then run:

```sh
make new P=<name> FQBN=<fqbn> SIM_FQBN=<sim_fqbn>
# or with label/purpose/baud:
./scripts/new-project.sh <name> <fqbn> <sim_fqbn> "<Board label>" "<purpose>" <baud>
```

Then:
- Write `docs/ideas/<date>-<name>.md` from the template.
- Write an ADR when the board or a part is new to the repo.
- Fill in `projects/<name>/docs/README.md` (BOM, wiring), `diagram.json`, and the native tests.
- Run `make test` and `make build`, and report the results.

Full guide: [docs/howto/new-project.md](docs/howto/new-project.md).

## Toolchain

- `arduino-cli` (Homebrew). Cores: `arduino:renesas_uno`, `arduino:avr`. Add others on demand.
- `wokwi-cli` in `~/bin` (needs `WOKWI_CLI_TOKEN`).
- `clang++` (Xcode CLT) for native tests.
- Check with `make doctor`.

## Docs map

- Ideas (not committed to): `docs/ideas/`
- Decisions: `docs/adr/` (numbered; never rewrite an accepted ADR, supersede it with a new one)
- How-tos: `docs/howto/`
- System and workflow architecture: `docs/architecture/`
- Per-project hardware and software: `projects/<p>/docs/`
- Blog drafts: `docs/blog/` (the user's, don't edit unless asked)
