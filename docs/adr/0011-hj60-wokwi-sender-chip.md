# 0011. hj60_coolant: custom Wokwi chip for the mode B sender circuit

- Status: Accepted
- Date: 2026-11-01
- Projects: hj60_coolant

## Context

- Mode B switches 3.3 V through Rref 100 Ω into the sender with GPIO26. The firmware also reads the sender wire with
  the excitation off, to find a backfeed.
- Wokwi does not simulate resistor networks. A test sketch showed that a Wokwi potentiometer **ignores its VCC pin**.
  With VCC on GPIO26, the output was 1865 mV with GPIO26 low and also with GPIO26 high. Thus potentiometers cannot model the
  switched excitation, and the firmware reports FAULT-BACKFEED in the sim.
- `wokwi-cli chip compile` builds custom chips (C → WebAssembly). It downloads the WASI SDK once.

## Options

1. **Custom chip** that computes Vtop/2 and Vs/2 from a temperature control, only while EN (GPIO26) is high.
2. **Sim-only firmware change** to skip the backfeed check: not allowed (AGENTS.md rule 1, no sim hooks).
3. **Test mode B only natively**: loses the real-library and real-core check of the new circuit.

## Decision

Option 1. `emulator/wokwi/chips/hj60-sender.chip.c` with controls `temperature` (20–150 °C), `open` (0/1) and
`backfeed` (0–3.3 V). It uses the FSM Beta curve. `wokwi.toml` loads `chips/hj60-sender.chip.wasm`.

## Consequences

- The scenario sets the coolant temperature in °C, which reads like the truck.
- The chip found a real problem before the hardware. An open sender gave Vs ≈ 3 V. The ESP32 ADC compresses that
  voltage, so the open wire read as "8 °C, OK". The fix: Vs goes through the same 47k/47k divider as Vtop.
- The Wokwi ADC model has an offset of about +0.14 V. The sim reads 10–20 °C low at the hot end. The scenario checks
  states only. The real board uses `ADC_OFFSET_MV` from step M5.
- Rebuild the chip after a change: `wokwi-cli chip compile chips/hj60-sender.chip.c -o chips/hj60-sender.chip.wasm`.
  The `.wasm` and the downloaded `wokwi-api.h` are committed, so `make sim` does not need the SDK.
