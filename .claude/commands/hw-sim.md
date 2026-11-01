---
description: Run Wokwi simulation scenarios for a project
argument-hint: "[project] [scenario.yaml|--interactive]"
---
Project: first word of `$ARGUMENTS`, default `water_monitor`. Optional second word: scenario path relative to
`emulator/wokwi/`, or `--interactive`.

- If `make doctor` shows the token as MISS, stop and tell the user to get one at https://wokwi.com/dashboard/ci and
  put it in `.env` (`WOKWI_CLI_TOKEN=wok_...`). Never ask for the token in chat, and never read or print `.env`.
- Run `make sim P=<project> S=<scenario>`, or `make sim-live P=<project>`.
- Artifacts go to `build/<project>/wokwi/`: `*.serial.log` and the `*.oled.png` screenshot. Read the screenshot and
  describe what the OLED shows.
- Each run spends free-tier minutes (50 per 30 days). Don't loop it.
