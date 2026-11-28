---
title: Models & downloads
---

# Print files & models

Everything for **Clawd V2**: print files, the PCB, the editable source and
renders. Which file to print for which part is in the
[build guide](/setup#_3-print-the-case).

::: info Which PCB files
Order **rev A** (`clawd-v2-gerber.zip`): that is the board in use. **Rev B**
only adds `+ − BAT` marks next to the battery socket; it has not been ordered
yet, so look at it in a Gerber viewer first. Ordering notes are on
[V2 hardware](/v2#notes-for-ordering).
:::

## What V2 looks like

Drag to turn it, scroll to zoom, and press **Take apart** to see the parts.
Point at a part for its name.

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

| What | File |
| --- | --- |
| PCB to order | [Gerber/drill ZIP, rev A](/models/v2/pcb/clawd-v2-gerber.zip) |
| PCB with assembly files | [JLCPCB package: Gerbers, BOM, CPL, notes](/models/v2/pcb/clawd-v2-jlcpcb-prototype.zip) |
| PCB, next revision | [Gerber/drill ZIP, rev B](/models/v2/pcb/rev-b/clawd-v2-gerber-rev-b.zip) (not ordered yet) |
| Editable source | [`clawd-v2.ket`](/models/v2/clawd-v2.ket): open in [Ket](/v2#open-in-another-ket-project), the CAD/PCB tool it was made in |
| Notes | [V2 hardware & manufacturing](/v2) |

Every file:

<ModelDownloads />

## Legacy V1 files

The first, hand-wired Clawd. Kept for reference; its shells do not fit the V2
board.

- **`mascot-top.3mf` / `mascot-bottom.3mf`**: the two enclosure shells.
- **`mascot.FCStd` / `assembly.FCStd`**: FreeCAD sources.
- **`assembly_Page__.svg`**: drawing page.
- **`view.jpeg`**: V1 preview image.
