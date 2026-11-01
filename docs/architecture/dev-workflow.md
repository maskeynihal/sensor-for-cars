# Development workflow (agentic)

How an idea becomes code on a board, and who does what at each step.

## End to end

```mermaid
flowchart TD
  U([User: idea / change request]) --> Q{New project?}
  Q -- yes --> ASK[Agent asks: purpose, board, parts, power]
  ASK --> SCAF[scripts/new-project.sh<br/>idea note + ADRs]
  SCAF --> EDIT
  Q -- no --> READ[Agent reads AGENTS.md, project README,<br/>project docs, relevant ADRs]
  READ --> EDIT[Edit SIM COPY<br/>emulator/wokwi/&lt;p&gt;_sim/&lt;p&gt;_sim.ino]
  EDIT --> NT[make test<br/>native host tests]
  NT -- fail --> EDIT
  NT -- pass --> SB[make build-sim<br/>fits sim board? RAM %]
  SB -- fail --> EDIT
  SB -- pass --> TOK{WOKWI_CLI_TOKEN?}
  TOK -- yes --> WK[make sim<br/>Wokwi scenarios + OLED screenshot]
  WK -- fail --> EDIT
  TOK -- no --> NOTE[Report: Wokwi skipped]
  WK -- pass --> PR
  NOTE --> PR[make promote<br/>sim -> firmware, real-board compile, tests again]
  PR -- fail: auto-rollback --> EDIT
  PR -- pass --> DOCS[Update project docs / ADR if wiring,<br/>behaviour or decisions changed]
  DOCS --> REP[Agent reports: test counts, flash/RAM %,<br/>what wasn't verified]
  REP --> UP{User asks to upload?}
  UP -- yes --> FL[make upload PORT=...<br/>make monitor]
  FL --> HW([User checks real hardware])
  HW -- bug --> EDIT
  UP -- no --> DONE([Done; firmware ready])
```

## What each tier can and cannot prove

| | Native (`make test`) | Wokwi (`make sim`) | Real board |
|---|---|---|---|
| Runs | `.ino` + mocks on the Mac (clang++) | sim `.ino` on an emulated AVR/ESP32/Pico | firmware on the target |
| Cost | free, ~2 s | 50 free min / 30 days | your time |
| Thresholds, state machines, timing math | ✅ | ✅ | ✅ |
| Sensor-failure paths (NaN, unplugged) | ✅ easy | ⚠️ partial | ❌ hard to stage |
| Real library code, I2C/OneWire traffic | ❌ mocked | ✅ | ✅ |
| Pixels on the OLED | ❌ text only | ✅ screenshot | ✅ |
| RAM/stack limits | ❌ | ✅ (of the sim board) | ✅ |
| Board-specific (R4 WiFi radio, LED matrix, RA4M1 timing) | ❌ | ❌ | ✅ |

## Code flow between folders

```mermaid
flowchart LR
  subgraph emulator["projects/&lt;p&gt;/emulator/"]
    SIM["wokwi/&lt;p&gt;_sim/&lt;p&gt;_sim.ino<br/>(where edits are made)"]
    SHIM["wokwi/shims/*<br/>sim-only libs"]
    DIA["wokwi/diagram.json<br/>+ scenarios/*.yaml"]
    NAT["native/test_*.cpp"]
  end
  subgraph fw["projects/&lt;p&gt;/firmware/"]
    FW["&lt;p&gt;/&lt;p&gt;.ino<br/>(uploadable)"]
  end
  LIB["libraries/ (vendored)"]
  MOCK["tools/native/mocks"]

  SIM -- "make promote (copy)" --> FW
  SIM --> |arduino-cli SIM_FQBN<br/>+ --library shims| HEX["build/&lt;p&gt;/sim/*.hex"] --> WOKWI["wokwi-cli"]
  DIA --> WOKWI
  SHIM --> HEX
  LIB --> HEX
  LIB --> BIN["build/&lt;p&gt;/firmware/*.bin"]
  FW --> |arduino-cli FQBN| BIN --> BOARD["Real board via USB"]
  SIM --> NB["build/&lt;p&gt;/native/test_sim"]
  FW --> NB2["build/&lt;p&gt;/native/test_firmware"]
  NAT --> NB & NB2
  MOCK --> NB & NB2
```

## Rules this enforces

- Firmware is only ever overwritten after the sim copy passes its tests, and only kept if the firmware then
  compiles for the real board and passes the tests too (`scripts/promote.sh`).
- Native tests always run on **both** copies, so a hand edit to firmware still gets tested.
- `make drift` is empty when the two copies are in sync.
