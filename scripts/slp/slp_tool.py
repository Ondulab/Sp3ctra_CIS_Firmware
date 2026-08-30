#!/usr/bin/env python3
"""Sp3ctra Link (SLP v1) host-side test tool - drives a real CIS without the VST.

    slp_tool.py discover [--broadcast 255.255.255.255] [--seconds 3]
    slp_tool.py stat     --ip 192.168.100.1 [--seconds 3]
    slp_tool.py hid      --ip ... [--seconds 10] [--rate 200]      # buttons edges + IMU
    slp_tool.py lines    --ip ... [--seconds 10]                    # LINE datagrams, loss, rate
    slp_tool.py led      --ip ... --led 0 --b1 100 [--t1 0 --g1 0 --b2 0 --t2 0 --g2 0 --blink 0 --no-local]
    slp_tool.py overlay  --ip ... "Speed=12 ms:0.6" "Gain=-3.0 dB:0.5b" [--ttl 1500] [--hold 3]
    slp_tool.py clear    --ip ...
    slp_tool.py cfg      --ip ... get dpi oversampling ...
    slp_tool.py cfg      --ip ... set oversampling=8 stream_when_unbound=1
    slp_tool.py cal      --ip ... cis|imu

Every command that needs a session does HELLO -> BIND -> ... -> UNBIND.
"""
import argparse
import os
import random
import select
import socket
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import slp  # noqa: E402

HOST_VERSION = (0, 0, 1)


def now_ms():
    return int(time.monotonic() * 1000)


class Link:
    """Control channel client (one session)."""

    def __init__(self, ip, port=slp.CTRL_PORT, stream_port=slp.STREAM_PORT, hid_rate=0, verbose=True):
        self.ip, self.port = ip, port
        self.stream_port, self.hid_rate = stream_port, hid_rate
        self.seq = slp.Seq()
        self.session = random.getrandbits(32) or 1
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.bind(("", 0))
        self.sock.settimeout(0.5)
        self.verbose = verbose
        self.ack = None
        self.announce = None
        self.last_ping = 0

    def send(self, data):
        self.sock.sendto(data, (self.ip, self.port))

    def recv(self, want_type, timeout=0.5):
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            try:
                data, addr = self.sock.recvfrom(2048)
            except socket.timeout:
                return None
            h = slp.parse_hdr(data)
            if not h:
                continue
            if h[0] == slp.ERROR:
                rt, code = slp.parse_error(data)
                print(f"  ! device ERROR in reply to {slp.TYPE_NAMES.get(rt, rt)}: code {code}")
                if want_type == slp.ERROR:
                    return data
                continue
            if h[0] == want_type:
                return data
        return None

    def hello(self, retries=3):
        for _ in range(retries):
            self.send(slp.build_hello(self.seq.next(), HOST_VERSION))
            data = self.recv(slp.ANNOUNCE, 0.5)
            if data:
                self.announce = slp.parse_announce(data)
                return self.announce
        return None

    def bind(self, retries=10):
        for _ in range(retries):
            self.send(slp.build_bind(self.seq.next(), self.session, self.stream_port,
                                     slp.STREAM_UNICAST_TO_SENDER, "0.0.0.0", self.hid_rate, 0,
                                     slp.FEAT_ALL, HOST_VERSION))
            data = self.recv(slp.BIND_ACK, slp.BIND_RETRY_MS / 1000)
            if data:
                self.ack = slp.parse_bind_ack(data)
                return self.ack
        return None

    def ping(self):
        self.send(slp.build_ping(self.seq.next(), self.session, now_ms()))
        self.last_ping = time.monotonic()
        data = self.recv(slp.PONG, 0.5)
        return slp.parse_pong(data) if data else None

    def keepalive(self):
        """Call regularly from loops: sends a PING every PING_PERIOD_MS (no wait)."""
        if time.monotonic() - self.last_ping >= slp.PING_PERIOD_MS / 1000:
            self.send(slp.build_ping(self.seq.next(), self.session, now_ms()))
            self.last_ping = time.monotonic()

    def unbind(self):
        self.send(slp.build_unbind(self.seq.next(), self.session))

    def open(self):
        a = self.hello()
        if not a:
            sys.exit(f"no ANNOUNCE from {self.ip}:{self.port} (device off, wrong IP, firewall?)")
        if self.verbose:
            print_announce(a, self.ip)
        ack = self.bind()
        if not ack:
            sys.exit("no BIND_ACK")
        if ack.status != slp.BIND_OK:
            sys.exit(f"BIND refused: status {ack.status} ({'BUSY' if ack.status == 1 else 'error'})"
                     + (f" - in use by {a.bound_peer_ip}" if a.bound else ""))
        if self.verbose:
            print(f"  session 0x{self.session:08X} bound: {ack.dpi} DPI, {ack.pixels_per_line} px/line, "
                  f"{ack.fragment_count} x {ack.fragment_pixels} px fragments ({ack.line_packet_bytes} B), "
                  f"HID {ack.hid_rate_hz} Hz mask 0x{ack.hid_valid_mask:X}, timeout {ack.session_timeout_ms} ms")
        return ack

    def close(self):
        self.unbind()
        self.sock.close()


def print_announce(a, ip):
    print(f"{a.name}  {ip}  serial-hash={a.uid.hex()}  MAC={':'.join('%02X' % b for b in a.mac)}")
    print(f"  fw {a.fw_version[0]}.{a.fw_version[1]}.{a.fw_version[2]}  hw rev {a.hw_revision}  proto>={a.proto_min}"
          f"  ctrl {a.ctrl_port}  stream {a.stream_port}  features 0x{a.features:04X}")
    print(f"  {a.n_buttons} buttons, {a.n_leds} LEDs ({'RGB' if a.led_kind else 'mono'}), IMU kind {a.imu_kind}, "
          f"display {a.display_w}x{a.display_h}x{a.display_bpp}bpp, DPI {a.dpi} -> px {a.pixels_at_dpi}, "
          f"max {a.line_rate_max} lps / {a.hid_rate_max} Hz")
    print(f"  state: {'BOUND to ' + a.bound_peer_ip if a.bound else 'free'}")


def stream_socket(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    if hasattr(socket, "SO_REUSEPORT"):
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
    s.bind(("", port))
    s.setblocking(False)
    return s


# ---- commands ------------------------------------------------------------------
def cmd_discover(args):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.bind(("", 0))
    s.settimeout(0.2)
    seq = slp.Seq()
    seen = {}
    end = time.monotonic() + args.seconds
    next_hello = 0
    while time.monotonic() < end:
        if time.monotonic() >= next_hello:
            for target in args.broadcast:
                s.sendto(slp.build_hello(seq.next(), HOST_VERSION), (target, slp.CTRL_PORT))
            next_hello = time.monotonic() + 1.0
        try:
            data, addr = s.recvfrom(2048)
        except socket.timeout:
            continue
        h = slp.parse_hdr(data)
        if h and h[0] == slp.ANNOUNCE:
            a = slp.parse_announce(data)
            if addr[0] not in seen:
                seen[addr[0]] = a
                print_announce(a, addr[0])
    if not seen:
        print("no device answered (same subnet? firewall? device powered?)")
    return 0


def cmd_stat(args):
    link = Link(args.ip)
    link.open()
    for _ in range(max(1, int(args.seconds * 2))):
        p = link.ping()
        if p:
            rtt = now_ms() - p.host_time_ms
            print(f"  PONG uptime {p.uptime_ms / 1000:.1f} s  lines {p.lines_sent}  {p.line_rate_lps} lps  "
                  f"cal {p.cal_state}/{p.cal_progress}%  {p.temp_c:.1f} C  flags 0x{p.link_flags:X}  rtt {rtt} ms")
        else:
            print("  no PONG")
        time.sleep(0.5)
    link.close()
    return 0


def cmd_hid(args):
    link = Link(args.ip, hid_rate=args.rate)
    ack = link.open()
    rx = stream_socket(link.stream_port)
    last_seq = None
    last_btn = None
    count = 0
    lost = 0
    t0 = time.monotonic()
    last_print = t0
    end = t0 + args.seconds
    print(f"  listening HID on UDP {link.stream_port} for {args.seconds} s (press the buttons, move the device)")
    while time.monotonic() < end:
        link.keepalive()
        r, _, _ = select.select([rx], [], [], 0.05)
        if not r:
            continue
        while True:
            try:
                data, addr = rx.recvfrom(2048)
            except BlockingIOError:
                break
            h = slp.parse_hid(data)
            if not h:
                continue
            count += 1
            if last_seq is not None and h.seq != ((last_seq + 1) & 0xFFFFFFFF):
                lost += (h.seq - last_seq - 1) & 0xFFFFFFFF
            last_seq = h.seq
            if last_btn is None:
                last_btn = list(h.button_seq)
            for i in range(h.button_count):
                if h.button_seq[i] != last_btn[i]:
                    edges = (h.button_seq[i] - last_btn[i]) & 0xFFFFFFFF
                    state = "PRESSED " if h.button_state & (1 << i) else "released"
                    print(f"  SW{i + 1} {state} (seq {h.button_seq[i]}, +{edges} edge{'s' if edges > 1 else ''})")
                    last_btn[i] = h.button_seq[i]
            if time.monotonic() - last_print >= 0.5:
                last_print = time.monotonic()
                el = last_print - t0
                print(f"  acc {h.acc[0]:+.2f} {h.acc[1]:+.2f} {h.acc[2]:+.2f} g  gyro {h.gyro[0]:+6.1f} {h.gyro[1]:+6.1f} "
                      f"{h.gyro[2]:+6.1f} dps  {h.temp_c:.1f} C  btn 0b{h.button_state:03b}  "
                      f"{count / el:.0f} Hz  lost {lost}")
    link.close()
    print(f"  {count} HID datagrams, {lost} lost, expected ~{ack.hid_rate_hz} Hz")
    return 0


def cmd_lines(args):
    link = Link(args.ip)
    ack = link.open()
    rx = stream_socket(link.stream_port)
    frags = {}
    complete = 0
    incomplete = 0
    datagrams = 0
    lost = 0
    last_seq = None
    t0 = time.monotonic()
    end = t0 + args.seconds
    last_print = t0
    while time.monotonic() < end:
        link.keepalive()
        r, _, _ = select.select([rx], [], [], 0.05)
        if not r:
            continue
        while True:
            try:
                data, addr = rx.recvfrom(4096)
            except BlockingIOError:
                break
            f = slp.parse_line(data)
            if not f:
                continue
            datagrams += 1
            if last_seq is not None and f.seq != ((last_seq + 1) & 0xFFFFFFFF):
                lost += (f.seq - last_seq - 1) & 0xFFFFFFFF
            last_seq = f.seq
            got = frags.setdefault(f.line_id, set())
            got.add(f.fragment_index)
            if len(got) == f.fragment_count:
                complete += 1
                del frags[f.line_id]
            # lines older than 8 ids with missing fragments are lost
            for lid in [k for k in frags if (f.line_id - k) > 8]:
                incomplete += 1
                del frags[lid]
            if time.monotonic() - last_print >= 1.0:
                last_print = time.monotonic()
                el = last_print - t0
                print(f"  {complete / el:6.1f} lines/s  {datagrams / el:7.1f} datagrams/s  complete {complete}  "
                      f"incomplete {incomplete}  lost datagrams {lost}  period {f.period_us} us  "
                      f"last {f.pixel_count} px @ {f.pixel_offset} ({f.fragment_index + 1}/{f.fragment_count})")
    link.close()
    print(f"  layout {ack.pixels_per_line} px = {ack.fragment_count} x {ack.fragment_pixels}; "
          f"{complete} complete lines, {incomplete} incomplete, {lost} datagrams lost")
    return 0


def cmd_led(args):
    link = Link(args.ip)
    link.open()
    cmd = slp.LedCmd(args.b1, args.g1, args.t1, args.b2, args.g2, args.t2, args.blink,
                     slp.LED_NO_LOCAL_PRESS if args.no_local else 0)
    link.send(slp.build_led_set(link.seq.next(), {args.led: cmd}))
    print(f"  LED{args.led + 1} <- {cmd}")
    for _ in range(4):
        time.sleep(0.5)
        link.keepalive()
    link.close()
    return 0


def parse_overlay_item(text):
    """'Label=value:0.5' or 'Label=value:0.5b' (bipolar) or 'Label=value' (no bar). '!' prefix = highlight."""
    flags = 0
    if text.startswith("!"):
        flags |= slp.OVL_HIGHLIGHT
        text = text[1:]
    label, _, rest = text.partition("=")
    value, _, bar = rest.partition(":")
    norm = 0xFFFF
    if bar:
        if bar.endswith("b"):
            flags |= slp.OVL_BIPOLAR
            bar = bar[:-1]
        norm = max(0, min(65535, int(float(bar) * 65535)))
    return slp.OverlayItem(label, value, norm, flags)


def cmd_overlay(args):
    link = Link(args.ip)
    link.open()
    items = [parse_overlay_item(t) for t in args.items]
    if items and not any(i.flags & slp.OVL_HIGHLIGHT for i in items):
        items[0].flags |= slp.OVL_HIGHLIGHT
    link.send(slp.build_overlay(link.seq.next(), items, args.ttl))
    print(f"  overlay ttl {args.ttl} ms: {items}")
    end = time.monotonic() + args.hold
    while time.monotonic() < end:
        time.sleep(0.2)
        link.keepalive()
    link.close()
    return 0


def cmd_clear(args):
    link = Link(args.ip)
    link.open()
    link.send(slp.build_oled_clear(link.seq.next()))
    link.close()
    return 0


def cmd_cfg(args):
    link = Link(args.ip)
    link.open()
    if args.op == "get":
        ids = [slp.CFG_IDS[n] if n in slp.CFG_IDS else int(n) for n in args.items] or sorted(slp.CFG_IDS.values())
        msg = slp.build_cfg(link.seq.next(), ids[:slp.CFG_MAX_ITEMS], set_values=False)
    else:
        items = []
        for it in args.items:
            name, _, val = it.partition("=")
            cid = slp.CFG_IDS[name] if name in slp.CFG_IDS else int(name)
            items.append((cid, slp.CFG_TYPES.get(cid, slp.CFG_U32), slp.cfg_pack_value(cid, val)))
        msg = slp.build_cfg(link.seq.next(), items[:slp.CFG_MAX_ITEMS], set_values=True)
    link.send(msg)
    data = link.recv(slp.CFG_REPLY, 3.0)   # SET may recalibrate the IMU (~1.2 s)
    if not data:
        print("  no CFG_REPLY")
    else:
        for cid, ctype, flags, value in slp.parse_cfg_reply(data):
            fl = slp.cfg_flags_str(flags)
            print(f"  {slp.CFG_NAMES.get(cid, cid):>20} = {slp.cfg_format_value(ctype, value)}" + (f"   [{fl}]" if fl else ""))
    link.close()
    return 0


def cmd_cal(args):
    link = Link(args.ip)
    link.open()
    kind = slp.CAL_CIS if args.kind == "cis" else slp.CAL_IMU
    link.send(slp.build_cal_start(link.seq.next(), kind))
    print(f"  {args.kind.upper()} calibration requested; watching PONG for {args.seconds} s")
    end = time.monotonic() + args.seconds
    while time.monotonic() < end:
        p = link.ping()
        if p:
            print(f"  cal_state {p.cal_state}  progress {p.cal_progress}%  flags 0x{p.link_flags:X}")
        time.sleep(0.5)
    link.close()
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("discover"); p.add_argument("--broadcast", nargs="+", default=["255.255.255.255"]); p.add_argument("--seconds", type=float, default=3); p.set_defaults(fn=cmd_discover)

    def with_ip(name):
        q = sub.add_parser(name); q.add_argument("--ip", required=True); return q

    p = with_ip("stat"); p.add_argument("--seconds", type=float, default=3); p.set_defaults(fn=cmd_stat)
    p = with_ip("hid"); p.add_argument("--seconds", type=float, default=10); p.add_argument("--rate", type=int, default=0); p.set_defaults(fn=cmd_hid)
    p = with_ip("lines"); p.add_argument("--seconds", type=float, default=10); p.set_defaults(fn=cmd_lines)
    p = with_ip("led")
    p.add_argument("--led", type=int, default=0); p.add_argument("--b1", type=int, default=100); p.add_argument("--g1", type=int, default=0)
    p.add_argument("--t1", type=int, default=0); p.add_argument("--b2", type=int, default=0); p.add_argument("--g2", type=int, default=0)
    p.add_argument("--t2", type=int, default=0); p.add_argument("--blink", type=int, default=0); p.add_argument("--no-local", action="store_true")
    p.set_defaults(fn=cmd_led)
    p = with_ip("overlay"); p.add_argument("items", nargs="+"); p.add_argument("--ttl", type=int, default=1500); p.add_argument("--hold", type=float, default=2); p.set_defaults(fn=cmd_overlay)
    p = with_ip("clear"); p.set_defaults(fn=cmd_clear)
    p = with_ip("cfg"); p.add_argument("op", choices=["get", "set"]); p.add_argument("items", nargs="*"); p.set_defaults(fn=cmd_cfg)
    p = with_ip("cal"); p.add_argument("kind", choices=["cis", "imu"]); p.add_argument("--seconds", type=float, default=12); p.set_defaults(fn=cmd_cal)

    args = ap.parse_args()
    sys.exit(args.fn(args))


if __name__ == "__main__":
    main()
