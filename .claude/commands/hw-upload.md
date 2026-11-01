---
description: Compile and flash firmware to a connected board (user must ask)
argument-hint: "[project] [port]"
---
Run this only because the user asked. Project: first word of `$ARGUMENTS` (default `water_monitor`). Port: the
second word, if given.

1. `make drift P=<project>`. If the sim copy is ahead, ask whether to promote first.
2. `arduino-cli board list`. Pick the port that matches the project's FQBN. If it's ambiguous, ask.
3. `make upload P=<project> PORT=<port>`.
4. Ask the user to watch `make monitor P=<project> PORT=<port>`. `arduino-cli monitor` is interactive and blocks.
   You may run `arduino-cli monitor -p <port> -c baudrate=<BAUD>` in the background briefly if they want you to read it.
