"""Talks to the board over USB Serial for self-testing.

The firmware takes line commands:

    s          screenshot: "SNAP <w> <h>" + RGB565, high byte first
    k1, k2     press KEY1 (new planet) / KEY2 (next speed)
    perf       toggle a line every second: fps, paint and push time, free RAM
    acc        toggle accelerometer lines at 10 Hz: ax ay az, turn, climb
    st         one status line: seed, speed, camera, free RAM

    PY=~/.platformio/penv/bin/python
    $PY tools/serial_cmd.py monitor 10
    $PY tools/serial_cmd.py snap screen.png
    $PY tools/serial_cmd.py send perf --wait 5

The port is opened so that DTR/RTS never pass through the state that
resets the chip (see open_port(), from M5VoiceRecorder via M5TalkingTom).

Needs only pyserial, which ships with PlatformIO.
"""
import argparse
import glob
import struct
import sys
import time
import zlib

import serial


def find_port():
    ports = sorted(glob.glob("/dev/cu.usbmodem*"))
    if not ports:
        sys.exit("No /dev/cu.usbmodem* port found; is the board connected?")
    return ports[0]


def open_port(port=None):
    """Opens the port without resetting the board.

    The USB-Serial-JTAG resets the chip while RTS is high and DTR is low.
    macOS raises both lines when the port opens; pyserial then applies DTR
    before RTS, so clearing both passes through exactly that reset state.
    Keeping DTR high until RTS is down avoids it: (1,1) -> (1,0) -> (0,0).
    """
    link = serial.Serial()
    link.port = port or find_port()
    link.baudrate = 115200
    link.timeout = 0.05
    link.dtr = True
    link.rts = False
    link.open()
    link.dtr = False
    return link


def send(link, line):
    link.write((line + "\n").encode())
    link.flush()


class Reader:
    """Splits the board's output into lines, keeping what a deadline cut
    off for the next call."""

    def __init__(self, link, echo=None):
        self.link = link
        self.partial = b""
        self.echo = echo  # a function called with every line, or None

    def line(self, deadline, want=None):
        """Next line; with `want`, skips lines until one starts with it."""
        while time.time() < deadline:
            chunk = self.link.read(1)
            if not chunk:
                continue
            if chunk == b"\n":
                text = self.partial.decode(errors="replace").rstrip("\r")
                self.partial = b""
                if self.echo:
                    self.echo(text)
                if want is None or text.startswith(want):
                    return text
                continue
            self.partial += chunk
        return None

    def exact(self, size, deadline):
        data = b""
        while len(data) < size:
            if time.time() > deadline:
                raise TimeoutError("transfer cut short: %d of %d bytes" % (len(data), size))
            data += self.link.read(size - len(data))
        return data


def write_png(path, width, height, pixels, scale=3):
    rows = []
    for y in range(height):
        row = bytearray()
        for x in range(width):
            i = 2 * (y * width + x)
            v = (pixels[i] << 8) | pixels[i + 1]
            r, g, b = (v >> 11) & 0x1F, (v >> 5) & 0x3F, v & 0x1F
            row += bytes(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))) * scale
        rows += [bytes(row)] * scale

    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    raw = b"".join(b"\x00" + row for row in rows)
    header = struct.pack(">IIBBBBB", width * scale, height * scale, 8, 2, 0, 0, 0)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header)
                + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def snap(link, reader, out, scale=3):
    send(link, "s")
    deadline = time.time() + 10
    head = reader.line(deadline, want="SNAP ")
    if head is None:
        raise TimeoutError("no SNAP answer")
    _, w, h = head.split()
    w, h = int(w), int(h)
    write_png(out, w, h, reader.exact(w * h * 2, deadline), scale)


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    m = sub.add_parser("monitor")
    m.add_argument("seconds", type=float)
    s = sub.add_parser("snap")
    s.add_argument("out")
    s.add_argument("--scale", type=int, default=3)
    c = sub.add_parser("send")
    c.add_argument("line")
    c.add_argument("--wait", type=float, default=1.0)
    args = ap.parse_args()

    link = open_port()
    reader = Reader(link, echo=lambda line: print("  | " + line))
    if args.cmd == "monitor":
        deadline = time.time() + args.seconds
        while time.time() < deadline:
            reader.line(deadline)
    elif args.cmd == "snap":
        reader.echo = None
        snap(link, reader, args.out, args.scale)
        print("screenshot -> " + args.out)
    elif args.cmd == "send":
        send(link, args.line)
        deadline = time.time() + args.wait
        while time.time() < deadline:
            reader.line(deadline)


if __name__ == "__main__":
    main()
