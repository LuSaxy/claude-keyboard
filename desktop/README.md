# Clawd Control (Desktop)

Cross-platform desktop app to configure Clawd over Bluetooth LE.

Built with **Tauri v2 + Rust**. All Bluetooth work is native Rust (`btleplug`) —
there is **no Web Bluetooth**. The web view is only the UI and talks to Rust
through `invoke`.

## Features

- Scan and connect to Clawd over BLE.
- Set the external LED brightness.
- Remap Button 1 and Button 2 (keyboard or media keys).
- Change the BLE/USB device name.
- Reset the keymap or name to firmware defaults.

## Requirements

- Rust toolchain (`cargo`).
- Tauri system dependencies for your OS — see
  https://tauri.app/start/prerequisites/
  - Linux: WebKitGTK + `libbluetooth`/BlueZ.
  - Windows: WebView2 (preinstalled on Windows 10/11).
  - macOS: Xcode command line tools.
- The Tauri CLI: `cargo install tauri-cli --version "^2"`.

## Run

```sh
cargo tauri dev
```

Or build a release binary/bundle:

```sh
cargo tauri build
```

### Windows build

This repo is meant to be developed in WSL and built/run on **Windows** (WebView2
is preinstalled there). On Windows, install the Tauri CLI and run `cargo tauri
build`; the installer ends up in `src-tauri/target/release/bundle/`.

### Icons

The full platform icon set in `src-tauri/icons/` (PNG sizes, Windows `.ico`,
macOS `.icns`, and Windows Store logos) was generated with the Tauri CLI:

```sh
cargo tauri icon path/to/clawd-logo.png
```

Re-run that command whenever you change the logo; it overwrites the files in
`src-tauri/icons/`. Use a square 1024x1024 RGBA PNG for best results.

## Building for other platforms

Short version:

- **Windows ← Linux**: possible with `cargo-xwin` (NSIS installer + `.exe`).
- **macOS ← Linux**: **not supported** by Tauri (needs the Apple SDK, WebKit,
  and code signing). Use a real Mac or a macOS CI runner.
- **Everything, reliably**: GitHub Actions with native runners (see below).

### Recommended: GitHub Actions (Windows + macOS + Linux)

This repo ships `.github/workflows/desktop.yml`. It builds the app natively on
`windows-latest`, `macos-latest` (Intel + Apple Silicon), and `ubuntu-22.04`
using `tauri-apps/tauri-action`.

- Trigger it manually from the Actions tab (`workflow_dispatch`), or
- push a tag like `v0.1.0` to build + attach artifacts to a release.

This is the only practical way to get a macOS build without owning a Mac.

### Linux → Windows (cross-compile with cargo-xwin)

Produces the `.exe` and the NSIS installer (the `.msi`/WiX bundle cannot be
cross-compiled — build that on Windows).

```sh
# one-time setup
rustup target add x86_64-pc-windows-msvc
cargo install --locked cargo-xwin
cargo install --locked tauri-cli --version "^2"
sudo apt-get install -y clang lld llvm nsis   # Debian/Ubuntu

# build
cd desktop
cargo tauri build --runner cargo-xwin --target x86_64-pc-windows-msvc --bundles nsis
```

Output: `src-tauri/target/x86_64-pc-windows-msvc/release/`.

> You are on WSL: the simplest alternative is to install Rust + the Tauri CLI on
> the **Windows** side and run `cargo tauri build` there directly. WebView2 is
> already present on Windows 10/11, so no extra system libraries are needed.

### Linux → macOS

Not supported. `osxcross`-style hacks do not work with Tauri's WebKit + signing
requirements. Options:

- Build on a real Mac: `cargo tauri build` (Tauri auto-detects the arch; use
  `--target universal-apple-darwin` for a universal binary).
- Use the GitHub Actions workflow above (recommended).

You can also run the Rust binary directly (static UI is served from `ui/`):

```sh
cd src-tauri
cargo run
```

## Pairing notes

The LED, keymap, and name characteristics require an **encrypted (bonded) link**,
exactly like nRF Connect. Pair Clawd at the OS level first:

- **Windows**: pair `clawd` from Bluetooth settings; confirm on the device
  (Button 1) when the LED turns magenta.
- **Linux (BlueZ)**: pair/trust with `bluetoothctl` (`pair`, `trust`, `connect`),
  confirming on the device, then use this app to read/write.
- **macOS**: pair from System Settings → Bluetooth.

After bonding, use **Scan → select → Connect** in the app.

## Layout

| Path | Contents |
| --- | --- |
| `ui/` | Static HTML/CSS/JS front end (calls Rust via `invoke`). |
| `src-tauri/src/clawd.rs` | BLE UUIDs and protocol constants (mirror the firmware). |
| `src-tauri/src/ble.rs` | Native BLE backend + Tauri commands. |
| `src-tauri/src/main.rs` | Tauri app entry point. |
