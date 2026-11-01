# 0010. hj60_coolant: ILI9488 3.5" TFT, DIYables library, ILI9341 shim in Wokwi

- Status: Accepted
- Date: 2026-11-01
- Projects: hj60_coolant

## Context

- The owner's dashboard design is full colour, 6 panels, about 3:2. The 1.3" SH1106 (128x64, monochrome) cannot
  show it.
- The owner chose an ILI9488 3.5" 480x320 SPI TFT.
- Wokwi has no ILI9488 part. It has an ILI9341 (320x240).
- `libraries/` already has `DIYables_TFT_SPI` 1.0.3. It drives ILI9341, ILI9488 and ST7789 with one Adafruit_GFX base
  class (`DIYables_TFT_SPI`). The ILI9488 driver sends RGB666 (3 bytes for each pixel), as the ILI9488 SPI mode needs.
- `TFT_eSPI` is also vendored, but its pin setup is in a header inside the library. AGENTS.md does not allow edits to
  `libraries/`.

## Options

1. **DIYables_TFT_SPI** for the real ILI9488, and a sim shim that maps the class to its ILI9341 driver.
2. **TFT_eSPI**: faster, but needs a setup header in the library or build flags for each board.
3. **A different display that Wokwi has** (ILI9341 2.8"): no shim, but smaller than the owner's choice.

## Decision

Option 1.

- The sketch includes `<DIYables_TFT_SPI.h>` and uses `DIYables_ILI9488_SPI`.
- The sim shim (`emulator/wokwi/shims/DIYables_TFT_SPI_SimShim/DIYables_TFT_SPI.h`) includes the real ILI9341 header
  and defines `#define DIYables_ILI9488_SPI SimShim_ILI9488`. `SimShim_ILI9488` is a subclass of
  `DIYables_ILI9341_SPI` that forces 240x320. The shim uses a different class name, so its symbols do not collide with
  the real ILI9488 driver at link time.
- The UI uses a 480x320 design grid and scales every coordinate and text size to `width()` and `height()`. Thus the
  same sketch draws on 480x320 (truck) and 320x240 (Wokwi).

## Consequences

- The Wokwi screen is smaller and its text scaling is coarser than the real screen. Check the real layout on the
  hardware.
- A full redraw of the ILI9488 takes about 0.1–0.15 s. The firmware redraws only the values that change.
- The native tests use a TFT mock that records the text at each cursor position.
- The SH1106 shim (ADR 0005) is no longer used by hj60_coolant.
