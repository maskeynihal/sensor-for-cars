# Promote the sim copy to firmware

```sh
make drift P=water_monitor     # what differs
make promote P=water_monitor   # gated copy
```

`scripts/promote.sh` runs these steps:
1. Native tests on the sim copy. It stops on a failure.
2. Compile the sim copy for `SIM_FQBN`.
3. Back up the firmware `.ino` to `build/<p>/<p>.ino.bak`, then copy the sim `.ino` over the firmware.
4. Compile the firmware for the real `FQBN` and run the native tests on it. **If either fails, the backup is
   restored.**

Then review `git diff projects/<p>/firmware` and upload.

## When firmware must be edited by hand

Some code can't run on the sim board, e.g. UNO R4 WiFi radio (`WiFiS3`) or the LED matrix. Options, best first:
1. Make it compile on both boards with a board check the Arduino core provides
   (`#if defined(ARDUINO_UNOR4_WIFI)`). That keeps the files identical and is still valid uploadable code.
2. Edit firmware by hand, and make the same edit in the sim copy so `make drift` stays clean. Note it in the
   commit message.

Never put shims, mocks, or a `SIM` define in firmware.
