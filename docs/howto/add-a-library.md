# Add an Arduino library

`libraries/` is vendored and read-only. To add a library:

```sh
arduino-cli lib search "<keyword>"              # find the exact name + latest version
arduino-cli lib install "<Library Name>@<ver>"  # installs into the sketchbook libraries dir
ls "$(arduino-cli config get directories.user)/libraries"
cp -R "$(arduino-cli config get directories.user)/libraries/<Folder>" libraries/
# also copy any dependency it pulled in that libraries/ doesn't already have
```

Then:
1. Commit it on its own: `vendor: add <Library> <version>`.
2. Add a mock in `tools/native/mocks/<Header>.h` that covers the calls the sketch uses. Put any inputs it needs in
   `sim.h` and reset them in `sim::reset()` in `arduino_mock.cpp`.
3. Check that the Wokwi part exists. If it doesn't, write a shim plus an ADR (see ADR 0005).
4. Write an ADR if it's a meaningful choice (e.g. picking between two display libraries).

Upgrades work the same way: replace the folder, commit, and re-run `make test` + `make build` for every project.
