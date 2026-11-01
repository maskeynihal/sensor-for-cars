@AGENTS.md

## Claude Code specifics

- Slash commands in `.claude/commands/`:
  - `/new-project`: asks the questions, then scaffolds
  - `/hw-test [project]`: native tests, plus Wokwi if a token is set
  - `/hw-sim [project]`: Wokwi scenarios
  - `/hw-promote [project]`: gated sim → firmware
  - `/hw-upload [project] [port]`: flash the real board, only when the user asks
  - `/adr <title>`: new ADR
  - `/idea <title>`: new idea note
- Use AskUserQuestion for the new-project questions. Ask them in one batch, not one by one.
- When you report results, quote the `N passed, M failed` lines and the flash/RAM % from the build. If Wokwi or upload
  didn't run, say so plainly.
