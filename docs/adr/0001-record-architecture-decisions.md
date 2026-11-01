# 0001. Record architecture decisions

- Status: Accepted
- Date: 2026-11-01
- Projects: all

## Context

Agents and humans both change this repo. Choices about boards, parts, libraries, and tooling get lost in chat
history, and an agent without that history makes them again, differently.

## Decision

Use short ADRs in `docs/adr/`, numbered `NNNN-kebab-title.md`, in the format of `0000-template.md`. An accepted ADR is
never rewritten. A new ADR supersedes it. Ideas that aren't committed to yet go in `docs/ideas/`, not here.

## Consequences

Every new board, part, library, or emulator workaround needs an ADR. `/adr <title>` creates one.
