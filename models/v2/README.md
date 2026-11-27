# Clawd V2 — Ket prototype

This directory supersedes the legacy FreeCAD/3MF geometry for **new prototype
builds**. The old files in `models/` are retained for reference, not regenerated
from this model. Do not mix V1 and V2 shell halves or PCBs.

**Prototype, not production-approved.** Electrical DRC passes, but the physical
slide-switch actuator fit is unresolved. Do not order a batch before checking
the switch and RF/assembly details below.

## Files

| File | Use |
| --- | --- |
| `clawd-v2.ket` | Editable Ket source: version 1 command archive, including parameters, CAD, PCB, components, routing, checks and materials. |
| `clawd-v2-assembly.step` | Assembly geometry backup / CAD exchange. Includes electronics and keycaps: do **not** print as one object. Some electronic parts are simplified blocks, not vendor shapes. |
| `stl/clawd-v2-bottom.stl`, `stl/clawd-v2-top.stl` | Printable shell halves. The top shell carries the L1/R1 flexure levers and the raised MODE/POWER labels. |
| `stl/clawd-v2-eye-left.stl`, `stl/clawd-v2-eye-right.stl` | Yellow translucent eye covers. |
| `stl/clawd-v2-usb-bezel.stl` | Clear USB surround/status-light window. |
| `stl/clawd-v2-battery-plate.stl` | Drop-in plate over the LiPo (rests on ledges in the bottom shell; keeps the key sockets off the battery, and two posts carry the hot-swap sockets when a key is pressed). Print flat, posts up, one colour. |
| `stl/clawd-v2-bottom-small-lipo.stl`, `stl/clawd-v2-battery-plate-small-lipo.stl` | `kucuk_pil` configuration: the same parts for a 30 × 37 × 4 mm LiPo (leads from the front-left corner). |
| `3mf/clawd-v2-top-print.3mf` | Two-material print: orange top shell + clear eye covers, one object per body. |
| `3mf/clawd-v2-bottom-print.3mf` | Two-material print: bottom shell + clear USB/LED bezel, one object per body. |
| `3mf/clawd-v2-battery-plate.3mf`, `3mf/clawd-v2-battery-plate-small-lipo.3mf` | Battery plate (normal and `kucuk_pil` LiPo) as 3MF; print flat, one colour. |
| `pcb/clawd-v2-gerber.zip` | Gerber layers and Excellon drill only, for bare-board quoting. This is the board that was ordered (rev A). |
| `pcb/rev-b/clawd-v2-gerber-rev-b.zip` | **Rev B, not ordered, not reviewed:** rev A plus `+`, `−` and `BAT` silkscreen at the battery socket J1 (back side). Exported with a newer Ket, so the GND pours and back mask differ slightly from rev A; check it in a Gerber viewer before ordering. |
| `pcb/clawd-v2-jlcpcb-prototype.zip` | Gerbers, drill, JLCPCB-style BOM/CPL and a local preflight/order guide. No order was submitted. |
| `pcb/bom.csv`, `pcb/cpl.csv` | Assembly component list and placement; verify all substitutions, rotations and sides before ordering. |
| `pcb/manufacturing.json` | Requested quantity/specification, readiness report and unresolved warnings. |
| `verification.json` | Full CAD/check results and SHA-256 hashes of generated geometry/ZIPs. |
| `SHA256SUMS` | Checksums for all release files except this checksum file. |

All dimensions are **millimetres**. The PCB is two layers, 1.6 mm thick, with a
59.8 × 36 mm bounding box (including the two mounting tabs). It is a polygon,
not a rectangular board. The case body is approximately 64.2 mm wide and 25.5 mm
high; keycaps project above it.

## Open in another Ket project

Use a current Ket build with `.ket` import/export support:

1. Create an empty project.
2. Choose **Project file → Import Ket into empty project**.
3. Select `clawd-v2.ket` and wait for imported geometry to load.
4. Run `run_checks` and `run_drc` before making a new export.

The file stores prepared footprints and route results, not user accounts,
server IDs or credentials. Imported vendor STEP models and textures are still
external URL/ID references: first opening needs network access. STEP/STL files
are supplied as geometry backups if those external sources change.

## Printing and assembly

- Print **one of each STL**, independently. They retain assembly coordinates;
  use the slicer's place-on-bed/orientation controls for each part.
- Orange shells (the L1/R1 levers are part of the top shell), yellow translucent eye covers, and a
  clear bezel. The clear parts fill their seats exactly (zero clearance), so the
  intended way is a two-material print from the `3mf/` files. When printing
  them separately and gluing, sand or scale them slightly for a dry fit.
- The LiPo sits in a low cradle on the floor; the battery plate drops in on top
  of it (pillars stop 0.1 mm under the PCB). The LiPo leads run along the left
  wall over the plate into the J1 pocket.
- **Socket supports:** two 7 × 3.5 mm posts on the plate stop **0.15 mm** under
  the bodies of the hot-swap sockets K1/K2 (parameter `sock_gap`). A key press
  flexes the PCB that far, then lands on the plate instead of the sockets'
  solder joints. The gap is computed from the socket's 3D model (body 2.0 mm
  below the PCB): if your sockets sit lower and the plate rocks or presses the
  board up, sand the posts; if a press still flexes the board noticeably,
  raise them (`sock_gap` in the Ket file, or a 0.2 mm shim on top).
- The PCB is located by two **pegs on the top shell** (Ø 2.0 mm) that drop into
  the 2.2 mm holes of the board's two arm tabs when the shells close, so the
  board cannot slide (±0.1 mm). If a peg is too tight for your printer, open it
  with a 2 mm drill or sand it lightly; do not force the shells together.
- **L1 / R1 are part of the top shell.** Each is a paddle in the back wall that
  hangs on a thin 0.8 mm leaf (a flexure) towards the outer corner; pressing the
  paddle bends the leaf and a finger under it presses the side switch (0.1 mm
  rest gap, 0.2 mm switch travel). Nothing slides, so it cannot jam. Print the
  top shell upside down as usual and do not let supports fill the slots around
  the paddles; clean the slots with a knife if they fused.
- Raised icons around the two slide switches (0.6 mm lines, 0.5 mm high):
  a **battery** above POWER and **swap arrows** above MODE on the top shell;
  **I · O** under POWER and the **USB trident · Bluetooth rune** under MODE on
  the bottom shell. Seen from behind, the knob pushed towards **I** / **USB** is
  battery on / USB keyboard; towards **O** / **Bluetooth** is battery off /
  Bluetooth only.
- **Snap fit:** five 8 mm bumps on the inside of the top shell skirt (one on
  each arm, one on each side wall, one at the front) click into matching
  grooves in the bottom shell lip. Closing pushes them 0.2 mm into the lip, so
  press until it clicks all round; to open, pull the halves apart at an arm.
  If it is too tight on your printer, sand the bumps lightly; if too loose,
  print the top shell with 0.05 mm less horizontal expansion.
- The L1/R1 paddles are centred on the split line: about 6.5 mm above it on
  the top shell and 2.5 mm below it, reaching into a matching window in the
  bottom shell. The lower part is 1 mm thin, so even fully pressed it stays
  0.9 mm away from the board edge.
- Openings are kept small: the slide switch windows end 0.25 mm above the
  switch body and the flexure leaf sits flush with the outer wall. Only a
  0.4 mm slit remains around each paddle so it can move.
- Print **both shells again** for this revision (labels and snap grooves are on
  the bottom shell, the matching bumps on the top shell). Do not mix with older
  prints.
- The upper shell contacts the PCB tabs nominally with **zero gap** and the
  lower shell supports them, so the board is clamped up/down; the pegs stop it
  sliding. Zero-gap rigid contact is not a spring: PCB and print tolerances can
  still leave the board slightly loose vertically. If it rattles, put a thin
  insulating pad (0.2–0.5 mm tape) on top of each tab; do not place pads over
  components or copper.
- A nominal **40 × 35 × 5 mm** LiPo envelope fits the geometry. The clearance
  to the JST body is only **0.3 mm** and below the cell only **0.2 mm**: account
  for wrapper, leads, protection PCB, insulation and swelling. Do not force a
  tight battery into the case; choose a smaller cell if needed.

## Required review before fabrication

1. **Slide switches SW1/SW2:** current 3D representations are bounding boxes,
   not accurate MSK12C02G20-B actuator geometry. Pockets are 4.6 × 2.6 mm, but
   height, travel and reach through the 1.8 mm wall are **not verified** against
   a physical part. Measure a sample or supply an accurate model, then adjust
   the openings/add actuator extensions before printing final shells.
   [Manufacturer drawing](https://atta.szlcsc.com/upload/public/pdf/source/20240521/5FF18B3A8C7D819B395CD1239172FE0B.pdf).
2. **RF:** verify the XIAO antenna keepout and copper/enclosure clearance against
   Seeed's recommendations. The present electrical DRC does not certify RF
   performance or antenna keepouts.
3. **Assembly:** XIAO, sockets and battery connector on the bottom, switches and
   eye LEDs on top. Have the assembler confirm that the XIAO module is
   supported and review CPL rotations. Hand assembly may be necessary.
4. **Firmware:** the firmware in `program/xiao_ble_keyboard/` reads the two
   shoulder switches (L1 = SW3 on D4, R1 = SW4 on D5; default Shift+Tab / Tab)
   and the control app can remap them. Test them on the assembled board.
5. **USB/battery/mechanics:** test the actual cable overmould, charging/status
   LED visibility, button travel, peg fit and battery wiring/insulation.
6. Inspect the Gerber ZIP in an independent/manufacturer viewer, including the
   polygon outline, all drill tools, copper, mask and assembly placements.

## Quote / order handoff

For JLCPCB, upload `pcb/clawd-v2-gerber.zip` at <https://jlcpcb.com/quote>.
Select 2 layers, 1.6 mm thickness and the quantity you want; finish, copper
weight, solder mask and other process choices are not specified by the CAD.
For assembly, submit BOM/CPL separately and review the visual placement proof.

PCBWay can receive the same Gerber/drill ZIP at
<https://www.pcbway.com/orderonline.aspx>; verify BOM/CPL column mapping with
them. The richer JLCPCB ZIP's `ORDERING.md` and `manufacturing.json` are for
human review, not instructions that a manufacturer's website will apply
automatically. This release contains no live quote and no paid order.

## Validation / provenance

Source: Ket project `clawd_v2`, applied operations 1–314 (enclosure, battery
plate and configurations). The PCB has not changed since operation 283, and the
files in `pcb/` are the ones from that release. **115 built features, 65/65
checks, no unrouted connections.** The current Ket DRC is stricter than the one
used for the Gerbers: it reports 7 `hole_to_copper` findings (non-plated peg
holes of SW1–SW4 and the key sockets against copper, two tracks 0.2 mm from
socket holes). Review them before ordering another batch. These algorithmic
checks do not resolve the physical review items above. See `verification.json`
for the complete results.

Models inherit [CC BY-NC-SA 4.0](../LICENSE) and the attribution in the root
README. External switch/keycap/vendor models retain their upstream licenses;
the assembly STEP contains imported vendor geometry.
