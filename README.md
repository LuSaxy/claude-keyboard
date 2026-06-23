# Claude Keyboard - Clawd

![Clawd keyboard preview](models/view.jpeg)

**Live site:** <https://lusaxy.github.io/claude-keyboard/> — landing page with the
preview, model downloads, a setup guide, the rendered firmware/README docs, and
the browser control panel (at `/app/`).

**Clawd Keyboard** is a small two-button keyboard built around the Seeed Studio XIAO
nRF52840. It can type over Bluetooth LE or USB-HID, run from a LiPo battery, and
sleep deeply when it is not in use.

It is meant to be a tiny, customisable shortcut pad: map the buttons to free
keys like `Enter`/`Down Arrow`, media controls, or your own app shortcuts.

## Highlights

- Two-button BLE + USB keyboard firmware for XIAO nRF52840.
- Default keys are `Enter` (button 1) and `Down Arrow` (button 2).
- Runtime BLE configuration for key mappings, device name, and the external LED.
- Confirmed BLE pairing, battery reporting, LiPo charging, and deep sleep.
- Browser control panel (Web Bluetooth) for LED, buttons, and name.
- Printable mascot-style enclosure and FreeCAD sources in `models/`.

## Components

- **Seeed Studio XIAO nRF52840** — the main microcontroller board, with BLE and USB support.
- **LiPo battery [150mAh - 300mAh]** — powers the XIAO and can be charged via USB.
- **2x Kailh Hot-swappable PCB Socket Hot Plug A** — allows easy replacement of the keyboard switches.
- **2x Universal Instantly Custom Double-Layer Keycaps with Transparent Cover Cap** — keycaps for the buttons.
- **Dip Switch 2** — on/off and usb/ble mode selection.
- **2x Led Yellow** — LED for custom (EYES), better square shape or little bit file.
- **2x 300-390 Ohm Resistors** — for the LED.
- **2x Mechanical Keyboard Switches** — the two buttons for the keyboard.
- **3D-printed clawd enclosure** — top and bottom parts, with some transparent areas.

You need soldering and some 28AWG wire to connect the battery and switches to the XIAO.

## Repository

| Path | Contents |
| --- | --- |
| `program/xiao_ble_keyboard/` | Arduino firmware and detailed build/flash notes. |
| `Makefile` | Convenience targets that drive the site (`make`, `make docs-build`, …). |
| `docs/` | Self-contained VitePress project (its own `package.json`): landing, setup guide, models page, and the Web Bluetooth control app (`docs/.vitepress/theme/components/ClawdApp.vue`). |
| `docs/scripts/prepare.mjs` | Build helper: copies the model files into `docs/public/models` and generates the firmware/README doc pages from source. |
| `models/` | FreeCAD sources, printable 3MF files, drawing export, and preview image. |

### Develop the site

From the repository root:

```sh
make               # install deps (first run) and start the dev server
make docs-server   # same as `make` — start the dev server
make docs-build    # production build into docs/.vitepress/dist
make docs-preview  # build, then serve the production output
make docs-clean    # remove deps, build output, and generated pages
```

Or work inside `docs/` directly with `npm install` / `npm run dev|build|preview`.

`dev`/`build` first run `docs/scripts/prepare.mjs`, which pulls the model files
into `docs/public/models` and regenerates `docs/firmware.md` and `docs/readme.md`
from the source README files (both are git-ignored). GitHub Pages builds and
deploys the site via `.github/workflows/pages.yml` (Node + VitePress, with Git
LFS enabled so the model binaries and preview image are real files).

## Models

The current model assets are:

- `models/view.jpeg` - preview image used above.
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
- remap Button 1 and Button 2;
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
