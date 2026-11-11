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
| `stl/clawd-v2-bottom.stl`, `stl/clawd-v2-top.stl` | Printable shell halves. |
| `stl/clawd-v2-button-left.stl`, `stl/clawd-v2-button-right.stl` | Grey shoulder button caps. |
| `stl/clawd-v2-eye-left.stl`, `stl/clawd-v2-eye-right.stl` | Yellow translucent eye covers. |
| `stl/clawd-v2-usb-bezel.stl` | Clear USB surround/status-light window. |
| `pcb/clawd-v2-gerber.zip` | Gerber layers and Excellon drill only, for bare-board quoting. |
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
- Orange shells, grey shoulder buttons, yellow translucent eye covers, and a
  clear bezel. Fit transparent inserts into their seats with a suitable
  adhesive after a dry fit; no snap retention is claimed.
- The board has two optional screws, one in each arm. The upper shell contacts
  the PCB tabs nominally with **zero gap**, and the lower shell supports them.
  The lid has a **4.8 mm diameter × 2.5 mm high** clearance pocket above each
  optional screw head. Pilot holes are nominally 1.6 mm and board clearances
  2.2 mm. Select/verify screw length, head size and thread with a printed sample;
  screws are not included in the electronic BOM.
- Zero-gap rigid contact is not a spring or a latch: PCB and print tolerances
  can leave the board loose or apply excessive pressure, and the case halves
  must themselves remain securely closed. Test a physical prototype before
  relying on screwless retention; use screws or appropriately placed compliant
  insulating pads if needed. Do not place pads over components or copper.
- Install the shoulder caps before closing the two shells. Their operating
  fit and return action still need a physical prototype.
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
4. **Firmware:** this revision adds two shoulder switches. Existing firmware
   still documents a two-key keyboard; shoulder inputs require firmware work
   and are not claimed to work automatically with the old build.
5. **USB/battery/mechanics:** test the actual cable overmould, charging/status
   LED visibility, button travel, screw retention and battery wiring/insulation.
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

Source: Ket project `clawd_v2`, applied operations 1–283. Reopened from the
archived `.ket` file in a fresh kernel before export: **104 built features
(including board/part bodies), 42/42 checks, zero DRC violations and no
unrouted connections**. These algorithmic checks do not resolve the physical
review items above. See `verification.json` for the complete results.

Models inherit [CC BY-NC-SA 4.0](../LICENSE) and the attribution in the root
README. External switch/keycap/vendor models retain their upstream licenses;
the assembly STEP contains imported vendor geometry.
