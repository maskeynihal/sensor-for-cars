# Hardware dev loop. Every target takes P=<project> (default: water_monitor).
#   make test            native host tests (sim copy + firmware)
#   make sim             Wokwi scenarios (needs WOKWI_CLI_TOKEN)
#   make promote         sim copy -> firmware, gated by tests + builds
#   make upload PORT=..  flash the real board
# See docs/howto/ for details.

P ?= water_monitor
PORT ?=
S ?=
T ?=

.PHONY: help doctor doctor-install projects build build-fw build-sim test test-sim test-fw sim sim-live drift promote upload monitor new clean

help:
	@grep -E '^[a-z-]+:.*## ' $(MAKEFILE_LIST) | awk -F':.*## ' '{printf "  %-15s %s\n", $$1, $$2}'

doctor: ## check toolchain, cores, wokwi token
	@./scripts/doctor.sh
doctor-install: ## install missing cores / tools
	@./scripts/doctor.sh --install
projects: ## list projects
	@for d in projects/*/project.env; do basename $$(dirname $$d); done

build: ## compile sim copy + firmware
	@./scripts/build.sh $(P) all
build-fw: ## compile firmware for the real board
	@./scripts/build.sh $(P) firmware
build-sim: ## compile sim copy for the Wokwi board
	@./scripts/build.sh $(P) sim

test: ## native tests on sim copy + firmware (T=<filter>)
	@./scripts/test.sh $(P) all $(T)
test-sim: ## native tests on sim copy only
	@./scripts/test.sh $(P) sim $(T)
test-fw: ## native tests on firmware only
	@./scripts/test.sh $(P) firmware $(T)

sim: ## Wokwi scenarios (S=scenarios/x.yaml for one)
	@./scripts/sim.sh $(P) $(S)
sim-live: ## Wokwi free-run 60 s, serial to terminal, OLED screenshot
	@./scripts/sim.sh $(P) --interactive

drift: ## diff sim copy vs firmware
	@./scripts/drift.sh $(P)
promote: ## copy tested sim copy into firmware (gated)
	@./scripts/promote.sh $(P)

upload: ## flash firmware (PORT=/dev/cu.usbmodemXXXX, auto if one board)
	@./scripts/upload.sh $(P) $(PORT)
monitor: ## serial monitor (PORT=...)
	@./scripts/monitor.sh $(P) $(PORT)

new: ## scaffold project: make new P=name FQBN=... SIM_FQBN=...
	@./scripts/new-project.sh "$(P)" "$(FQBN)" "$(SIM_FQBN)"

clean: ## remove build/
	rm -rf build
