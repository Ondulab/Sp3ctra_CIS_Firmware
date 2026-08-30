#!/usr/bin/env python3
"""Simulated Sp3ctra CIS speaking SLP v1 - lets the VST be developed without hardware.

    slp_fake_device.py [--ctrl-port 55150] [--lps 200] [--dpi 400] [--name Sp3ctra-FAKE]

Answers HELLO / BIND / PING / UNBIND / CFG / CAL, prints every LED_SET and
OLED_OVERLAY it receives, and streams a synthetic moving pattern (LINE) plus a
HID flow (buttons + slowly rotating IMU) to the bound host at the negotiated
rate. Keyboard (stdin, one command per line):

    1 | 2 | 3      toggle button SW1..SW3
    p1 | p2 | p3   press-and-release (two edges)
    a x y z        set acceleration (g)          g x y z   set gyro (dps)
    q              quit
"""
import argparse
import math
import os
import random
import select
import socket
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import slp  # noqa: E402


class FakeDevice:
    def __init__(self, args):
        self.args = args
        self.ctrl = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.ctrl.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.ctrl.bind(("", args.ctrl_port))
        self.ctrl.setblocking(False)
        self.stream = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.seq_ctrl, self.seq_line, self.seq_hid = slp.Seq(), slp.Seq(), slp.Seq()
        self.uid = bytes(random.Random(args.name).getrandbits(8) for _ in range(12))
        self.mac = bytes([0x02, 0x53, 0x33]) + self.uid[:3]
        self.bound = None          # dict(session, peer, stream, hid_rate, last_ping)
        self.dpi = args.dpi
        self.cfg = {1: self.dpi, 2: 4, 3: 1, 4: 3, 5: 2, 6: 0, 7: 0, 8: 60, 9: 0.1, 10: 2.0,
                    20: "192.168.100.1", 21: "255.255.255.0", 22: "0.0.0.0", 23: "192.168.100.10",
                    24: slp.STREAM_PORT, 25: 1, 26: args.ctrl_port, 40: args.lps}
        self.buttons = [0, 0, 0]
        self.button_seq = [0, 0, 0, 0]
        self.acc = [0.0, 0.0, 1.0]
        self.gyro = [0.0, 0.0, 0.0]
        self.auto_imu = True   # gentle rotation until the keyboard sets a value
        self.line_id = 0
        self.lines_sent = 0
        self.cal_state, self.cal_progress, self.cal_until = 8, 0, 0
        self.t0 = time.monotonic()
        self.next_line = 0.0
        self.next_hid = 0.0

    # ---- helpers
    @property
    def pixels(self):
        return 3456 if self.dpi == 400 else 1728

    def uptime_ms(self):
        return int((time.monotonic() - self.t0) * 1000)

    def log(self, msg):
        print(f"[{self.uptime_ms() / 1000:8.2f}] {msg}", flush=True)

    def send_ctrl(self, data, addr):
        self.ctrl.sendto(data, addr)

    # ---- control
    def handle_ctrl(self, data, addr):
        h = slp.parse_hdr(data)
        if not h:
            return
        typ = h[0]
        if typ == slp.HELLO:
            self.send_ctrl(slp.build_announce(self.seq_ctrl.next(), self.uid, self.mac, name=self.args.name,
                                              ctrl_port=self.args.ctrl_port, bound=1 if self.bound else 0,
                                              peer=self.bound["peer"][0] if self.bound else "0.0.0.0"), addr)
            self.log(f"HELLO from {addr[0]}:{addr[1]} -> ANNOUNCE")
        elif typ == slp.BIND:
            f = slp.S_BIND.unpack_from(data, slp.HDR.size)
            session, stream_port, mode, _, mcast, hid_rate, dpi, want, hv, _ = f
            if self.bound and self.bound["peer"] != addr:
                self.send_ctrl(slp.build_bind_ack(self.seq_ctrl.next(), session, slp.BIND_BUSY), addr)
                self.log(f"BIND from {addr} refused: BUSY (bound to {self.bound['peer']})")
                return
            stream_ip = slp.ip_str(mcast) if mode == slp.STREAM_MULTICAST else addr[0]
            self.bound = dict(session=session, peer=addr, stream=(stream_ip, stream_port or slp.STREAM_PORT),
                              hid_rate=min(max(hid_rate or slp.DEFAULT_HID_RATE_HZ, 1), slp.MAX_HID_RATE_HZ),
                              last_ping=time.monotonic())
            fc = self.pixels // 288
            self.send_ctrl(slp.build_bind_ack(self.seq_ctrl.next(), session, slp.BIND_OK, self.dpi, self.pixels, fc, 288,
                                              self.bound["hid_rate"]), addr)
            self.log(f"BIND session 0x{session:08X} from {addr} host {hv[0]}.{hv[1]}.{hv[2]} -> stream {self.bound['stream']} HID {self.bound['hid_rate']} Hz")
        elif typ == slp.PING:
            session, host_time = slp.S_PING.unpack_from(data, slp.HDR.size)
            if not self.bound or self.bound["session"] != session:
                self.send_ctrl(slp.build_error(self.seq_ctrl.next(), typ, 3), addr)
                return
            self.bound["last_ping"] = time.monotonic()
            flags = slp.LINK_STREAMING | (slp.LINK_CAL_RUNNING if self.cal_state != 8 else 0)
            self.send_ctrl(slp.build_pong(self.seq_ctrl.next(), session, host_time, self.uptime_ms(), self.lines_sent,
                                          self.args.lps, self.cal_state, self.cal_progress, 31.5, flags), addr)
        elif typ == slp.UNBIND:
            (session,) = slp.S_UNBIND.unpack_from(data, slp.HDR.size)
            if self.bound and self.bound["session"] == session:
                self.log("UNBIND")
                self.bound = None
        elif typ == slp.LED_SET:
            mask, _ = slp.S_LED_SET_HEAD.unpack_from(data, slp.HDR.size)
            off = slp.HDR.size + slp.S_LED_SET_HEAD.size
            for i in range(slp.MAX_LEDS):
                if mask & (1 << i):
                    c = slp.LedCmd.unpack(data[off + i * 12:off + (i + 1) * 12])
                    self.log(f"LED{i + 1} <- b1={c.brightness_1} t1={c.time_1_ms} g1={c.glide_1} b2={c.brightness_2} "
                             f"t2={c.time_2_ms} g2={c.glide_2} blink={c.blink_count} flags={c.flags}")
        elif typ == slp.OLED_OVERLAY:
            ttl, count, layout = slp.S_OVL_HEAD.unpack_from(data, slp.HDR.size)
            off = slp.HDR.size + slp.S_OVL_HEAD.size
            items = [slp.OverlayItem.unpack(data[off + i * 28:off + (i + 1) * 28]) for i in range(count)]
            self.log(f"OVERLAY ttl={ttl} " + " | ".join(
                f"{'*' if it.flags & 2 else ''}{it.label}={it.value}" + (f" [{it.norm / 65535:.2f}{'b' if it.flags & 1 else ''}]" if it.norm != 0xFFFF else "")
                for it in items))
        elif typ == slp.OLED_CLEAR:
            self.log("OLED_CLEAR")
        elif typ in (slp.CFG_GET, slp.CFG_SET):
            count, _ = slp.S_CFG_HEAD.unpack_from(data, slp.HDR.size)
            off = slp.HDR.size + slp.S_CFG_HEAD.size
            reply = []
            for i in range(min(count, slp.CFG_MAX_ITEMS)):
                cid, ctype, _, value = slp.S_CFG_ITEM.unpack_from(data, off + i * 8)
                if cid not in self.cfg:
                    reply.append((cid, 0, slp.CFG_F_UNKNOWN, 0)); continue
                t = slp.CFG_TYPES[cid]
                flags = 0
                if typ == slp.CFG_SET:
                    if cid == 40:
                        flags = slp.CFG_F_READONLY | slp.CFG_F_REJECTED
                    else:
                        self.cfg[cid] = slp.cfg_format_value(t, value) if t == slp.CFG_IP4 else (
                            struct.unpack("<f", struct.pack("<I", value))[0] if t == slp.CFG_F32 else value)
                        if cid in (1, 20, 21, 22, 26):
                            flags = slp.CFG_F_REBOOT
                        if cid == 1:
                            self.dpi = value
                        self.log(f"CFG_SET {slp.CFG_NAMES.get(cid, cid)} = {self.cfg[cid]}")
                v = self.cfg[cid]
                packed = slp.cfg_pack_value(cid, str(v))
                reply.append((cid, t, flags, packed))
            self.send_ctrl(slp.build_cfg_reply(self.seq_ctrl.next(), reply), addr)
        elif typ == slp.CAL_START:
            kind, _ = slp.S_CAL.unpack_from(data, slp.HDR.size)
            self.log(f"CAL_START {'CIS' if kind == 0 else 'IMU'} (simulated 3 s)")
            self.cal_state, self.cal_progress, self.cal_until = 1, 0, time.monotonic() + 3
        else:
            self.send_ctrl(slp.build_error(self.seq_ctrl.next(), typ, 4), addr)

    # ---- stream
    def send_line(self):
        n = self.pixels
        fc = n // 288
        t = time.monotonic() - self.t0
        r = bytearray(n); g = bytearray(n); b = bytearray(n)
        for x in range(n):
            phase = x / n * 6.283 * 3 + t * 2.0
            r[x] = int(127 + 127 * math.sin(phase))
            g[x] = int(127 + 127 * math.sin(phase + 2.1))
            b[x] = int(127 + 127 * math.sin(phase + 4.2))
        period_us = int(1e6 / self.args.lps)
        for f in range(fc):
            off = f * 288
            self.stream.sendto(slp.build_line(self.seq_line.next(), self.line_id, off, 288, f, fc,
                                              r[off:off + 288], g[off:off + 288], b[off:off + 288], period_us),
                               self.bound["stream"])
        self.line_id += 1
        self.lines_sent += 1

    def send_hid(self):
        state = sum(1 << i for i, v in enumerate(self.buttons) if v)
        self.stream.sendto(slp.build_hid(self.seq_hid.next(), self.uptime_ms() * 1000, state, self.button_seq,
                                         self.acc, self.gyro, 31.5), self.bound["stream"])

    def toggle(self, i):
        self.buttons[i] ^= 1
        self.button_seq[i] += 1
        self.log(f"SW{i + 1} {'PRESSED' if self.buttons[i] else 'released'} (seq {self.button_seq[i]})")
        if self.bound:
            self.send_hid()   # immediate edge datagram, like the firmware

    def handle_stdin(self, line):
        line = line.strip()
        if line in ("1", "2", "3"):
            self.toggle(int(line) - 1)
        elif line in ("p1", "p2", "p3"):
            i = int(line[1]) - 1
            self.toggle(i); time.sleep(0.05); self.toggle(i)
        elif line.startswith("a "):
            self.acc = [float(v) for v in line.split()[1:4]]; self.auto_imu = False
        elif line.startswith("g "):
            self.gyro = [float(v) for v in line.split()[1:4]]; self.auto_imu = False
        elif line == "q":
            raise KeyboardInterrupt

    def run(self):
        self.log(f"{self.args.name} listening on UDP {self.args.ctrl_port}, {self.dpi} DPI, {self.args.lps} lps")
        stdin_ok = sys.stdin and not sys.stdin.closed and hasattr(sys.stdin, "fileno")
        try:
            while True:
                readers = [self.ctrl] + ([sys.stdin] if stdin_ok else [])
                r, _, _ = select.select(readers, [], [], 0.001)
                for s in r:
                    if s is self.ctrl:
                        data, addr = self.ctrl.recvfrom(2048)
                        self.handle_ctrl(data, addr)
                    else:
                        line = sys.stdin.readline()
                        if not line:
                            stdin_ok = False
                        else:
                            self.handle_stdin(line)
                now = time.monotonic()
                if self.bound and now - self.bound["last_ping"] > slp.SESSION_TIMEOUT_MS / 1000:
                    self.log("session timeout")
                    self.bound = None
                if self.cal_state != 8:
                    self.cal_progress = min(100, int((1 - (self.cal_until - now) / 3) * 100))
                    if now >= self.cal_until:
                        self.cal_state, self.cal_progress = 8, 100
                        self.log("calibration done")
                if self.bound:
                    if now >= self.next_line:
                        self.next_line = now + 1.0 / self.args.lps
                        self.send_line()
                    if now >= self.next_hid:
                        self.next_hid = now + 1.0 / self.bound["hid_rate"]
                        if self.auto_imu:   # gentle motion so continuous controls move
                            t = now - self.t0
                            self.gyro = [30 * math.sin(t), 20 * math.cos(t * 0.7), 0.0]
                            self.acc = [0.3 * math.sin(t * 0.5), 0.2 * math.cos(t * 0.3), 1.0]
                        self.send_hid()
        except KeyboardInterrupt:
            self.log("bye")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--ctrl-port", type=int, default=slp.CTRL_PORT)
    ap.add_argument("--lps", type=int, default=200)
    ap.add_argument("--dpi", type=int, choices=[200, 400], default=400)
    ap.add_argument("--name", default="Sp3ctra-FAKE")
    FakeDevice(ap.parse_args()).run()


if __name__ == "__main__":
    main()
