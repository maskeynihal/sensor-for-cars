# Set up the toolchain (macOS)

```sh
brew install arduino-cli
arduino-cli core update-index
arduino-cli core install arduino:renesas_uno arduino:avr   # real board + sim board

# Wokwi CLI (installs to ~/.wokwi, symlinks ~/bin/wokwi-cli)
curl -L https://wokwi.com/ci/install.sh | sh

# Native tests use clang++ from the Xcode command line tools
xcode-select --install   # if `clang++ --version` fails

make doctor              # check everything
```

## Wokwi token (only for `make sim`)

1. Sign in at https://wokwi.com/dashboard/ci and create a CI token. It starts with `wok_`.
2. Add `export WOKWI_CLI_TOKEN=wok_...` to `~/.zshrc` (never commit it).
3. Free tier: 50 simulated minutes per rolling 30 days, max 5 min per run.

## ESP32 / Pico cores (when a project needs them)

```sh
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli config add board_manager.additional_urls \
  https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32        # or rp2040:rp2040
```

Installed on this machine (2026-11-01): arduino-cli 1.5.1, renesas_uno 1.6.0, avr 1.8.8, wokwi-cli 0.29.1,
Apple clang 21.
