---
inclusion: always
---
# Tech

- Arduino C++ sketches built with `arduino-cli`. Libraries come from the vendored `libraries/` folder only.
- Real boards: see each `projects/*/project.env` (`FQBN`). Sim boards: `SIM_FQBN` (Wokwi-supported).
- Tests: native host tests (`clang++`, mocks in `tools/native/mocks`) via `make test P=<p>`. Wokwi via `make sim P=<p>`.
- Always use the Makefile targets. Don't hand-roll arduino-cli commands unless you are debugging the scripts.
