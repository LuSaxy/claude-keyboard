---
title: Models & downloads
---

# Models & downloads

The latest revision is **Clawd V2**, an unvalidated physical prototype. Download
the editable Ket file, printable STLs, STEP and PCB manufacturing files below.
V1 FreeCAD/3MF files are still available for reference; do not mix revisions.

**Before ordering:** SW1/SW2 actuator fit, XIAO antenna keepouts and the L1/R1
switch soldering (SW3/SW4 orientation, bridges under the body) need review. Clean CAD/DRC checks do not certify physical fit or DFM.

## What V2 looks like

Drag to turn it, scroll to zoom, and press **Take apart** to see the parts.
Hover a part for its name.

<Clawd3D />

Renders from the Ket model (the same geometry as the downloads below).

<div class="clawd-gallery">

![Clawd V2 from the front: two keys, the eyes and the arms](/models/v2/images/v2-front.webp)

![Clawd V2 from the back: USB-C window, the MODE and POWER switches and the L1/R1 shoulder buttons](/models/v2/images/v2-back.webp)

![Exploded view: keycaps, top shell, PCB with the XIAO and the keys, battery plate and bottom shell](/models/v2/images/v2-exploded.webp)

![Bottom shell with the battery plate over the LiPo](/models/v2/images/v2-bottom-plate.webp)

</div>

**Socket supports.** The battery plate has two posts that stop 0.15 mm under
the hot-swap sockets, so a key press lands on the plate instead of the
sockets' solder joints (section through the sockets):

![Section through the hot-swap sockets: posts on the battery plate under each socket](/models/v2/images/v2-section-sockets.webp)

### PCB

<div class="clawd-gallery">

![PCB top side: SW1/SW2 slide switches, SW3/SW4 shoulder switches, the eye LEDs and their resistors](/models/v2/images/pcb-top.webp)

![PCB bottom side: the XIAO, the Kailh hot-swap sockets K1/K2 and the battery socket J1 with + and - marks](/models/v2/images/pcb-bottom.webp)

</div>

Top side (left): the switches, eye LEDs and resistors. Bottom side (right):
the XIAO, the two hot-swap sockets and the battery socket J1. The `+ − BAT`
marks next to J1 are in **rev B** only; the boards ordered so far (rev A) do
not have them. J1 pin 1 (+) is next to the small silkscreen circle.

## Downloads

- [V2 assembly and manufacturing notes](/v2)
- [Editable Ket project](/models/v2/clawd-v2.ket)
- [Gerber/drill ZIP, rev A (ordered)](/models/v2/pcb/clawd-v2-gerber.zip)
- [Gerber/drill ZIP, rev B (+/− marks at J1, not ordered)](/models/v2/pcb/rev-b/clawd-v2-gerber-rev-b.zip)
- [JLCPCB prototype handoff](/models/v2/pcb/clawd-v2-jlcpcb-prototype.zip)

<ModelDownloads />

## Legacy files

- **`mascot-top.3mf` / `mascot-bottom.3mf`** — the two printable enclosure shells.
- **`mascot.FCStd` / `assembly.FCStd`** — editable FreeCAD sources.
- **`assembly_Page__.svg`** — exported drawing page.
- **`view.jpeg`** — the preview render used across the site.

More build photos and renders can be dropped into `models/` as the project grows.
