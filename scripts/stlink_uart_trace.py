#!/usr/bin/env -S scripts/.venv/bin/python
"""STLINK-V3 UART trace reader (macOS).

This script auto-detects the STLINK-V3 USB CDC serial endpoint and reads traces
at a high baudrate (default: 2,000,000). It also supports auto-reconnect when
the device disappears (e.g. unplug/reset).

Detection strategy:
- Prefer matching by explicit USB serial number when provided.
- Otherwise match by VID/PID (default: 0483:3753 for STLINK-V3).

Notes:
- On macOS, use /dev/cu.* for outgoing connections.
- This script prints raw bytes to stdout (decoded as UTF-8 with replacement).

"""

from __future__ import annotations

import argparse
import sys
import time
from dataclasses import dataclass

import serial
from serial.tools import list_ports


ST_VID_DEFAULT = 0x0483
ST_PID_DEFAULT = 0x3753


@dataclass(frozen=True)
class PortMatch:
    device: str
    description: str
    hwid: str
    serial_number: str | None
    vid: int | None
    pid: int | None


def _iter_ports() -> list[PortMatch]:
    ports: list[PortMatch] = []
    for p in list_ports.comports():
        ports.append(
            PortMatch(
                device=p.device,
                description=p.description or "",
                hwid=p.hwid or "",
                serial_number=getattr(p, "serial_number", None),
                vid=getattr(p, "vid", None),
                pid=getattr(p, "pid", None),
            )
        )
    return ports


def _normalize_serial(s: str) -> str:
    return "".join(ch for ch in s.strip() if ch.isalnum())


def _usbmodem_suffix_int(device: str) -> int | None:
    # Examples:
    # - /dev/cu.usbmodem2102 -> 2102
    # - /dev/tty.usbmodem2105 -> 2105
    base = device.rsplit("/", 1)[-1]
    for prefix in ("cu.usbmodem", "tty.usbmodem"):
        if base.startswith(prefix):
            tail = base[len(prefix) :]
            if tail.isdigit():
                return int(tail)
    return None


def _sort_candidates_for_macos(candidates: list[PortMatch], prefer_cu: bool) -> list[PortMatch]:
    # 1) Prefer /dev/cu.* over /dev/tty.*
    # 2) Then prefer the smallest usbmodem suffix (heuristic)
    def key(pm: PortMatch) -> tuple[int, int, str]:
        is_tty = 1 if "/tty." in pm.device else 0
        suffix = _usbmodem_suffix_int(pm.device)
        suffix_key = suffix if suffix is not None else 1_000_000_000
        # If prefer_cu is False, keep tty and cu together.
        is_tty_key = is_tty if prefer_cu else 0
        return (is_tty_key, suffix_key, pm.device)

    return sorted(candidates, key=key)


def _probe_port_has_data(device: str, baudrate: int, probe_s: float) -> bool:
    # Try to read a small amount of data for a short period.
    # This helps to auto-select the "talking" endpoint when multiple exist.
    try:
        with serial.Serial(
            port=device,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=probe_s,
            xonxoff=False,
            rtscts=False,
            dsrdtr=False,
        ) as ser:
            data = ser.read(64)
            return bool(data)
    except (OSError, serial.SerialException):
        return False


def find_stlink_port(
    *,
    usb_serial: str | None,
    vid: int,
    pid: int,
    prefer_cu: bool,
    baudrate: int,
    probe_s: float,
    verbose: bool,
) -> PortMatch | None:
    ports = _iter_ports()

    wanted_serial = _normalize_serial(usb_serial) if usb_serial else None

    # Match by USB serial (if requested), otherwise by VID/PID.
    candidates: list[PortMatch] = []
    if wanted_serial:
        for pm in ports:
            sn = _normalize_serial(pm.serial_number) if pm.serial_number else ""
            if sn and sn == wanted_serial:
                candidates.append(pm)
    else:
        candidates = [pm for pm in ports if pm.vid == vid and pm.pid == pid]

    if not candidates:
        return None

    candidates = _sort_candidates_for_macos(candidates, prefer_cu)

    # If multiple endpoints exist, try to pick the one that actually outputs data.
    if len(candidates) > 1 and probe_s > 0:
        if verbose:
            print("Multiple STLINK serial endpoints detected; probing for UART activity...", file=sys.stderr)
            for pm in candidates:
                print(f"  - {pm.device} ({pm.description})", file=sys.stderr)

        for pm in candidates:
            if _probe_port_has_data(pm.device, baudrate, probe_s):
                if verbose:
                    print(f"Selected active endpoint: {pm.device}", file=sys.stderr)
                return pm

        if verbose:
            print("No endpoint produced data during probe; falling back to preferred ordering.", file=sys.stderr)

    return candidates[0]


def open_serial(device: str, baudrate: int) -> serial.Serial:
    # No HW/SW flow control by default; keep it explicit.
    return serial.Serial(
        port=device,
        baudrate=baudrate,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.2,
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    )


def run(args: argparse.Namespace) -> int:
    usb_serial = args.serial
    vid = int(args.vid, 16) if isinstance(args.vid, str) else args.vid
    pid = int(args.pid, 16) if isinstance(args.pid, str) else args.pid

    while True:
        pm = find_stlink_port(
            usb_serial=usb_serial,
            vid=vid,
            pid=pid,
            prefer_cu=not args.allow_tty,
            baudrate=args.baudrate,
            probe_s=args.probe_s,
            verbose=args.verbose,
        )

        if pm is None:
            if not args.follow:
                print(
                    "No matching STLINK-V3 serial port found.",
                    file=sys.stderr,
                )
                return 2

            if args.verbose:
                print(
                    f"Waiting for STLINK-V3 port (VID:PID={vid:04x}:{pid:04x})...",
                    file=sys.stderr,
                )
            time.sleep(args.retry_s)
            continue

        if args.verbose:
            sn = pm.serial_number or "<unknown>"
            print(
                f"Connecting to {pm.device} ({pm.description}) VID:PID={pm.vid:04x}:{pm.pid:04x} SN={sn}",
                file=sys.stderr,
            )

        try:
            with open_serial(pm.device, args.baudrate) as ser:
                # Main read loop
                while True:
                    try:
                        chunk = ser.read(4096)
                    except serial.SerialException:
                        raise

                    if chunk:
                        # Preserve all bytes; decode for terminal display.
                        sys.stdout.write(chunk.decode("utf-8", errors="replace"))
                        sys.stdout.flush()

        except (OSError, serial.SerialException) as e:
            if not args.follow:
                print(f"Serial error: {e}", file=sys.stderr)
                return 3

            if args.verbose:
                print(f"Serial disconnected/error: {e}", file=sys.stderr)
                print("Reconnecting...", file=sys.stderr)

            time.sleep(args.retry_s)
            continue


def build_argparser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="stlink_uart_trace.py",
        description="Read UART traces via STLINK-V3 USB serial endpoint (auto-detect).",
    )

    p.add_argument(
        "--baudrate",
        type=int,
        default=2_000_000,
        help="UART baudrate (default: 2000000).",
    )

    p.add_argument(
        "--serial",
        type=str,
        default=None,
        help="USB serial number to match (recommended if you have multiple probes).",
    )

    p.add_argument(
        "--vid",
        type=lambda s: int(s, 16),
        default=ST_VID_DEFAULT,
        help="USB VID in hex (default: 0x0483).",
    )

    p.add_argument(
        "--pid",
        type=lambda s: int(s, 16),
        default=ST_PID_DEFAULT,
        help="USB PID in hex (default: 0x3753).",
    )

    p.add_argument(
        "--follow",
        action="store_true",
        default=True,
        help="Auto-reconnect if the port disappears (default: enabled).",
    )
    p.add_argument(
        "--no-follow",
        dest="follow",
        action="store_false",
        help="Disable auto-reconnect.",
    )

    p.add_argument(
        "--retry-s",
        type=float,
        default=1.0,
        help="Delay between retries when following (default: 1.0s).",
    )

    p.add_argument(
        "--allow-tty",
        action="store_true",
        help="Allow using /dev/tty.* devices (default: prefer /dev/cu.* only).",
    )

    p.add_argument(
        "--probe-s",
        type=float,
        default=0.8,
        help="When multiple endpoints exist, probe each port for this duration and select the one that outputs data (default: 0.8s).",
    )

    p.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Verbose status messages to stderr.",
    )

    return p


if __name__ == "__main__":
    sys.exit(run(build_argparser().parse_args()))
