---
description: Promote the tested sim copy into uploadable firmware
argument-hint: "[project]"
---
Project: `$ARGUMENTS`, default `water_monitor`.

1. `make drift P=<project>`. Show the user the diff summary.
2. `make promote P=<project>`. It is gated: sim tests → sim build → copy → firmware build → firmware tests, and it
   rolls back on failure.
3. Report the firmware flash/RAM % and the test counts. Tell the user the next step is
   `make upload P=<project> PORT=...` on real hardware. Don't upload unless they ask.
4. If the change touched wiring or behaviour, update `projects/<project>/docs/`.
