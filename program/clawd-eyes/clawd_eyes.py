#!/usr/bin/env python3
"""Drive Clawd's eye LEDs from scripts and apps on the computer.

Clawd must be plugged in over USB (the MODE switch can be on USB or BLE: the
serial port is there either way). Close any serial monitor first: only one
program can hold the port.

    clawd_eyes.py on | off | <0-255>
    clawd_eyes.py blink   [level] [cycle] [--for SECONDS]
    clawd_eyes.py breathe [level] [cycle] [--for SECONDS]
    clawd_eyes.py flash   [count] [level]

cycle is in tenths of a second (20 = 2 s). Without --for an effect keeps
running; with it the eyes go back to their steady brightness afterwards.
Examples:

    clawd_eyes.py breathe 255 20            # "thinking"
    clawd_eyes.py flash 3                   # "done"
    clawd_eyes.py blink 255 4 --for 10      # "needs attention" for 10 s

Needs pyserial (pip install pyserial). The port is found by USB id; give
--port /dev/cu.usbmodem101 (or COM5) to pick one by hand.
"""

import argparse
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("pyserial is missing: pip install pyserial")

SEEED_VID = 0x2886  # Seeed Studio XIAO nRF52840 (plain, Sense, Plus)


def find_port():
    ports = [p for p in list_ports.comports() if p.vid == SEEED_VID]
    if not ports:
        sys.exit("Clawd not found on USB. Plug it in, or give --port.")
    if len(ports) > 1:
        names = ", ".join(p.device for p in ports)
        sys.exit(f"More than one XIAO found ({names}); pick one with --port.")
    return ports[0].device


def build_command(args):
    what = args.effect
    if what in ("on", "off"):
        return f"led {what}"
    if what.isdigit():
        return f"led {what}"
    nums = [str(n) for n in args.numbers]
    if what in ("blink", "breathe"):
        level = nums[0] if len(nums) > 0 else "255"
        cycle = nums[1] if len(nums) > 1 else ("5" if what == "blink" else "20")
        return f"led {what} {level} {cycle} {args.seconds}"
    if what == "flash":
        return "led flash " + " ".join(nums[:2] or ["3"])
    sys.exit(f"unknown effect '{what}': use on, off, 0-255, blink, breathe or flash")


def main():
    parser = argparse.ArgumentParser(description="Clawd eye LEDs over USB serial.")
    parser.add_argument("effect", help="on, off, 0-255, blink, breathe or flash")
    parser.add_argument("numbers", nargs="*", type=int, help="level / cycle / count")
    parser.add_argument("--for", dest="seconds", type=int, default=0, help="stop after this many seconds")
    parser.add_argument("--port", help="serial port (default: found by USB id)")
    args = parser.parse_args()

    command = build_command(args)
    port = args.port or find_port()
    # Opening the port at 1200 baud would reset the board into the bootloader;
    # any other rate is fine.
    with serial.Serial(port, 115200, timeout=1) as ser:
        ser.reset_input_buffer()
        ser.write((command + "\n").encode())
        deadline = time.time() + 2
        while time.time() < deadline:
            line = ser.readline().decode(errors="replace").strip()
            if line == "ok":
                return
            if line.startswith("error"):
                sys.exit(line)
    sys.exit("No answer from Clawd: is it running the Clawd firmware?")


if __name__ == "__main__":
    main()
