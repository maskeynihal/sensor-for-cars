# Build and upload to the real board

```sh
make build-fw P=water_monitor                 # compile only (flash/RAM % printed)
arduino-cli board list                        # find the port, e.g. /dev/cu.usbmodem1101
make upload P=water_monitor PORT=/dev/cu.usbmodem1101
make monitor P=water_monitor PORT=/dev/cu.usbmodem1101   # Ctrl-C to quit
```

`make upload` with no `PORT` auto-picks the port when exactly one board matching the project's FQBN is
connected.

## Raw arduino-cli (what the scripts run)

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi --libraries ./libraries \
  --build-path build/water_monitor/firmware projects/water_monitor/firmware/water_monitor
arduino-cli upload --fqbn arduino:renesas_uno:unor4wifi --input-dir build/water_monitor/firmware \
  -p /dev/cu.usbmodemXXXX projects/water_monitor/firmware/water_monitor
```

## UNO R4 upload problems

- Port not listed: use a data USB-C cable. Double-tap RESET to enter the bootloader (the LED pulses), then re-run
  `board list`.
- `Permission denied`: close the Arduino IDE serial monitor. Only one program can hold the port.
