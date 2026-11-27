# Clawd site (VitePress project in docs/) and firmware (arduino-cli).

DOCS := docs

.DEFAULT_GOAL := help

## node_modules is (re)installed when the lockfile changes or it's missing.
$(DOCS)/node_modules: $(DOCS)/package.json $(DOCS)/package-lock.json
	cd $(DOCS) && npm install
	@touch $(DOCS)/node_modules

.PHONY: docs-server
docs-server: $(DOCS)/node_modules ## start the docs dev server
	cd $(DOCS) && npm run dev

.PHONY: docs-build
docs-build: $(DOCS)/node_modules ## build the static site into docs/.vitepress/dist
	cd $(DOCS) && npm run build

.PHONY: docs-preview
docs-preview: docs-build ## build, then serve the production output locally
	cd $(DOCS) && npm run preview

.PHONY: docs-clean
docs-clean: ## remove build output, caches, generated pages, and deps
	cd $(DOCS) && rm -rf node_modules .vitepress/dist .vitepress/cache public/models firmware.md readme.md

# ---- Firmware (arduino-cli) -------------------------------------------------
# The connected XIAO is found by arduino-cli: its serial port and board
# (plain, Sense or Plus) come from `arduino-cli board list`. Override with
#   make flash PORT=/dev/cu.usbmodem1101 FQBN=Seeeduino:nrf52:xiaonRF52840Sense

FIRMWARE := program/xiao_ble_keyboard
SELFTEST := program/clawd_selftest
BAUD     := 115200

# First connected Seeed XIAO nRF52840 (any variant): "<port> <fqbn>".
XIAO_DETECT = arduino-cli board list --format json 2>/dev/null | python3 -c 'import json,sys; \
ps=[p for p in json.load(sys.stdin).get("detected_ports",[]) for b in p.get("matching_boards",[]) if "nrf52:xiao" in b.get("fqbn","")]; \
print(ps[0]["port"]["address"], ps[0]["matching_boards"][0]["fqbn"]) if ps else None'
PORT ?= $(word 1,$(shell $(XIAO_DETECT)))
FQBN ?= $(word 2,$(shell $(XIAO_DETECT)))

.PHONY: check-board
check-board:
	@if [ -z "$(PORT)" ] || [ -z "$(FQBN)" ]; then \
		echo "No XIAO nRF52840 found. Plug it in (double-tap RESET if it does not show up),"; \
		echo "or give PORT=... FQBN=... by hand. arduino-cli board list:"; \
		arduino-cli board list; exit 1; fi
	@echo "Board: $(FQBN) on $(PORT)"

.PHONY: flash
flash: check-board ## build and flash the keyboard firmware to the connected XIAO
	arduino-cli compile --upload -p $(PORT) -b $(FQBN) $(FIRMWARE)

.PHONY: selftest
selftest: check-board ## flash the solder self-test, then show its report (Ctrl-C to stop)
	arduino-cli compile --upload -p $(PORT) -b $(FQBN) $(SELFTEST)
	@echo "Waiting for the board to restart..."
	@sleep 4
	@$(MAKE) --no-print-directory monitor PORT=$(PORT) FQBN=$(FQBN)
	@echo "Run 'make flash' to put the keyboard firmware back."

.PHONY: monitor
monitor: check-board ## open the serial monitor of the connected XIAO (Ctrl-C to stop)
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)

.PHONY: firmware-build
firmware-build: ## compile the keyboard firmware for plain, Sense and Plus (no board needed)
	@for b in xiaonRF52840 xiaonRF52840Sense xiaonRF52840Plus; do \
		echo "== $$b"; arduino-cli compile -b Seeeduino:nrf52:$$b $(FIRMWARE) | grep -E 'Sketch uses|error' || exit 1; \
	done

.PHONY: help
help: ## list the available targets
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2}'
