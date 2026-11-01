---
inclusion: always
---
# Structure

- `projects/<p>/firmware/`: uploadable. Changed only via `make promote`.
- `projects/<p>/emulator/`: where changes are made and tested (Wokwi sim copy plus native tests).
- `docs/ideas`, `docs/adr`, `docs/howto`, `docs/architecture`: repo-level docs. `projects/<p>/docs`: per-project docs.
- New project: ask for the processor and the purpose first, then `scripts/new-project.sh`.
