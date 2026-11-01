---
description: Run native tests (and Wokwi if a token is set) for a project
argument-hint: "[project] [test-filter]"
---
Project: first word of `$ARGUMENTS`, default `water_monitor`. Optional second word: test-name filter.

1. `make test P=<project> T=<filter>`. This runs host tests on the sim copy and on the firmware.
2. `make build P=<project>`. Report flash/RAM % for both boards. Warn if sim RAM is above 85%.
3. If `WOKWI_CLI_TOKEN` is set: `make sim P=<project>`. Otherwise say "Wokwi skipped: no token".
4. `make drift P=<project>`. Report whether the firmware is behind the sim copy.
Report the exact `N passed, M failed` lines. On a failure, show the failing CHECK and the serial tail, then diagnose.
