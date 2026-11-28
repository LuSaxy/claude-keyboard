# Claude Keyboard - Clawd

![Clawd V2 render](models/v2/images/v2-back.webp)

> **V2 is the current Clawd**: PCB, case and firmware are in use. The files and
> ordering notes are in [`models/v2/`](models/v2/README.md).

**Live site:** <https://lusaxy.github.io/claude-keyboard/>: the build guide,
parts list, model downloads, firmware reference and the browser control app.

**Clawd Keyboard** is a small two-key keyboard (plus two side shoulder buttons on V2) built around the Seeed Studio XIAO
nRF52840. It can type over Bluetooth LE or USB-HID, run from a LiPo battery, and
sleep deeply when it is not in use.

It is meant to be a tiny, customisable shortcut pad: map the buttons to free
keys like `Escape`/`Enter`, media controls, or your own app shortcuts.

## Highlights

- BLE + USB keyboard firmware for XIAO nRF52840: two keys and the V2 L1/R1 shoulder buttons.
- Default keys are `Escape` (left key), `Enter` (right key), `Shift+Tab` (L1) and `Tab` (R1).
- Runtime BLE configuration for key mappings, device name, and the external LED.
- Confirmed BLE pairing, battery reporting, LiPo charging, and deep sleep.
- Browser control panel (Web Bluetooth) for LED, buttons, and name.
- Printable mascot-style enclosure (V2: Ket source, STL/3MF, STEP and PCB files in `models/v2/`).

## Components

The full list with LCSC part numbers, links and a price table is in
[`docs/bom.md`](docs/bom.md) (on the site: **Parts list (BOM)**). In short:

- **Seeed Studio XIAO nRF52840 Sense** (or the plain XIAO nRF52840) — BLE, USB
  and the LiPo charger. **Not the Plus / Sense Plus**: their extra pads bridge
  the board's pads.
- **Clawd V2 PCB** with two Kailh hot-swap sockets, two slide switches (MODE,
  POWER), two side-push switches (L1, R1), two yellow LEDs with 330 Ω
  resistors and a JST PH 2.0 battery socket.
- **Protected 1S LiPo** with a JST PH 2.0 plug, up to 40 × 35 × 5 mm.
  **Measure the plug polarity before connecting it**: the board has no
  reverse-polarity protection, and a reversed cell damages the XIAO's charger.
- **Two MX switches and two 1u keycaps.**
- **The printed case** (orange shells, clear eyes and USB bezel, shoulder
  buttons, battery plate).

## Repository

| Path | Contents |
| --- | --- |
| `program/xiao_ble_keyboard/` | Arduino firmware and detailed build/flash notes. |
| `program/clawd_selftest/` | Solder self-test: finds bridged or open XIAO pins and checks every button. Flash it after soldering the module. |
| `docs/bom.md` | Parts list with links and prices, and the battery guide (choosing, polarity, charging, level). |
| `Makefile` | Convenience targets that drive the site (`make`, `make docs-build`, …). |
| `docs/` | Self-contained VitePress project (its own `package.json`): landing, setup guide, models page, and the Web Bluetooth control app (`docs/.vitepress/theme/components/ClawdApp.vue`). |
| `docs/scripts/prepare.mjs` | Build helper: copies the model files into `docs/public/models` and generates the firmware/README doc pages from source. |
| `models/v2/` | V2: Ket source, STL/3MF print files, STEP, PCB Gerbers/BOM/CPL, renders and the 3D-viewer model. |
| `models/` (top level) | Legacy V1: FreeCAD sources, printable 3MF files and drawing export. |

### Develop the site

From the repository root:

```sh
make               # list the targets
make docs-server   # install deps (first run) and start the dev server
make docs-build    # production build into docs/.vitepress/dist
make docs-preview  # build, then serve the production output
make docs-clean    # remove deps, build output, and generated pages
```

### Flash the firmware

With a XIAO on USB (port and board are detected; override with
`PORT=... FQBN=...`):

```sh
make flash           # build and flash the keyboard firmware
make selftest        # flash the solder self-test and show its report
make monitor         # serial monitor (s = print settings, R = wipe settings)
make firmware-build  # compile for plain, Sense and Plus, no board needed
```

Or work inside `docs/` directly with `npm install` / `npm run dev|build|preview`.

`dev`/`build` first run `docs/scripts/prepare.mjs`, which pulls the model files
into `docs/public/models` and regenerates `docs/firmware.md` and `docs/readme.md`
from the source README files (both are git-ignored). GitHub Pages builds and
deploys the site via `.github/workflows/pages.yml` (Node + VitePress, with Git
LFS enabled so the model binaries and preview image are real files).

## Models

### V2 (current)

Start with [`models/v2/README.md`](models/v2/README.md) for printing, opening the
`.ket` file in a new Ket project and ordering the PCB.

- [`clawd-v2.ket`](models/v2/clawd-v2.ket) — editable source, PCB and routing.
- [`clawd-v2-assembly.step`](models/v2/clawd-v2-assembly.step) — CAD geometry backup.
- [`clawd-v2-gerber.zip`](models/v2/pcb/clawd-v2-gerber.zip) — Gerber/drill package.
- [`clawd-v2-jlcpcb-prototype.zip`](models/v2/pcb/clawd-v2-jlcpcb-prototype.zip) — Gerbers, BOM and CPL for ordering an assembled board.

### Legacy V1

The original model assets are retained unchanged:

- `models/view.jpeg` - V1 preview image.
- `models/mascot-top.3mf` and `models/mascot-bottom.3mf` - printable enclosure parts.
- `models/assembly.FCStd` and `models/mascot.FCStd` - FreeCAD source files.
- `models/assembly_Page__.svg` - exported drawing page.

More build photos and renders can be dropped into `models/` as the project grows.

## Firmware

Start with the firmware README:

[`program/xiao_ble_keyboard/README.md`](program/xiao_ble_keyboard/README.md)

It covers wiring, pairing, custom key mappings, battery behaviour, Arduino IDE
setup, and upload troubleshooting.

## Control App

Open it on the live site at
<https://lusaxy.github.io/claude-keyboard/app>, or run the site locally with
`npm run dev`. It is a small **Web Bluetooth** control panel that runs in the
browser — no install. After pairing with **clawd**, it can:

- set the external LED brightness;
- remap Button 1, Button 2 and the L1 / R1 shoulder buttons;
- change the BLE/USB device name;
- reset the keymap or name to firmware defaults.

Use Chrome or Edge (desktop or Android), served over `https://` or
`http://localhost`. The app lives in
`docs/.vitepress/theme/components/ClawdApp.vue`.

## Git LFS

Files under `models/` are configured for Git LFS via `.gitattributes`, so large
CAD files, 3D-print files, renders, and photos stay out of normal Git history.

Before adding model files on a new machine, install Git LFS and run:

```sh
git lfs install
```

## Attribution

The Clawd enclosure design is based on the
[Claude Code Mascot Fidget Toy](https://www.myminifactory.com/object/3d-print-claude-code-mascot-fidget-toy-791307)
by **白林 (@user6961502839)** on MyMiniFactory, licensed under
[CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/).

## License

The firmware and site source code (`program/`, `docs/`, `Makefile`) are released
under the **MIT License** — see [`LICENSE`](LICENSE).

The 3D model files in `models/` are derived from the original design above and
are therefore released under **CC BY-NC-SA 4.0** — see
[`models/LICENSE`](models/LICENSE). This means the models are for
non-commercial use only, and any derivative works must carry the same license.
