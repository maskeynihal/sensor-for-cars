---
description: Write a new Architecture Decision Record
argument-hint: "<title>"
---
Create `docs/adr/NNNN-<kebab-title>.md`. NNNN is the next number after the highest in `docs/adr/`. Copy the format
from `docs/adr/0000-template.md`. Title: $ARGUMENTS

- Status starts as `Proposed`. Mark it `Accepted` only when the user agrees.
- Context: the facts that forced the decision (board limits, part availability, cost). Cite links.
- List the options considered, the decision, and the consequences (including what gets harder).
- If it replaces an older ADR, set the old one's Status to `Superseded by NNNN`. Change nothing else in it.
- Add a row to the index in `docs/adr/README.md`.
