#!/usr/bin/env python3
"""Sp3ctra Link Protocol (SLP) v1 - Python mirror of Common/Inc/sp3ctra_link.h.

Struct layouts and sizes are checked against the C header's static asserts
(see SIZES at the bottom). Keep this file in sync with the header.
"""
import struct
from dataclasses import dataclass, field
from typing import List, Optional

MAGIC = 0x5333
VERSION = 1
CTRL_PORT = 55150
STREAM_PORT = 55151
MAX_BUTTONS = 4
MAX_LEDS = 4
OVERLAY_MAX_ITEMS = 3
OVERLAY_LABEL_LEN = 14
OVERLAY_VALUE_LEN = 10
NAME_LEN = 16
CFG_MAX_ITEMS = 16
DEFAULT_HID_RATE_HZ = 200
MAX_HID_RATE_HZ = 1000
SESSION_TIMEOUT_MS = 3000
PING_PERIOD_MS = 500
HELLO_PERIOD_MS = 1000
BIND_RETRY_MS = 300
CTRL_MAX_BYTES = 160

# message types
HELLO, BIND, UNBIND, PING = 0x01, 0x02, 0x03, 0x04
LED_SET, OLED_OVERLAY, OLED_CLEAR = 0x10, 0x11, 0x12
CFG_GET, CFG_SET, CAL_START = 0x20, 0x21, 0x22
ANNOUNCE, BIND_ACK, PONG, CFG_REPLY, ERROR = 0x81, 0x82, 0x84, 0xA0, 0xFF
LINE, HID = 0xC0, 0xC1

TYPE_NAMES = {
    HELLO: "HELLO", BIND: "BIND", UNBIND: "UNBIND", PING: "PING", LED_SET: "LED_SET",
    OLED_OVERLAY: "OLED_OVERLAY", OLED_CLEAR: "OLED_CLEAR", CFG_GET: "CFG_GET", CFG_SET: "CFG_SET",
    CAL_START: "CAL_START", ANNOUNCE: "ANNOUNCE", BIND_ACK: "BIND_ACK", PONG: "PONG",
    CFG_REPLY: "CFG_REPLY", ERROR: "ERROR", LINE: "LINE", HID: "HID",
}

BIND_OK, BIND_BUSY, BIND_UNSUPPORTED, BIND_BAD_PARAM = 0, 1, 2, 3
STREAM_UNICAST_TO_SENDER, STREAM_MULTICAST = 0, 1
CAL_CIS, CAL_IMU = 0, 1

FEAT_LED_SET, FEAT_OLED_OVERLAY, FEAT_CFG, FEAT_CAL = 1 << 0, 1 << 1, 1 << 2, 1 << 3
FEAT_HID_BUTTONS, FEAT_HID_ACC, FEAT_HID_GYRO, FEAT_HID_TEMP = 1 << 8, 1 << 9, 1 << 10, 1 << 11
FEAT_ALL = (FEAT_LED_SET | FEAT_OLED_OVERLAY | FEAT_CFG | FEAT_CAL
            | FEAT_HID_BUTTONS | FEAT_HID_ACC | FEAT_HID_GYRO | FEAT_HID_TEMP)
HID_BUTTONS, HID_ACC, HID_GYRO, HID_TEMP = 1, 2, 4, 8

LED_NO_LOCAL_PRESS = 1
OVL_BIPOLAR, OVL_HIGHLIGHT = 1, 2
LINK_STREAMING, LINK_CAL_RUNNING = 1, 2
CFG_F_REBOOT, CFG_F_READONLY, CFG_F_UNKNOWN, CFG_F_REJECTED = 1, 2, 4, 8
CFG_U8, CFG_U16, CFG_U32, CFG_F32, CFG_IP4 = 0, 1, 2, 3, 4

CFG_IDS = {
    "dpi": 1, "oversampling": 2, "handedness": 3, "gyro_fs": 4, "accel_fs": 5,
    "gui_show_imu": 6, "gui_invert": 7, "screensaver_s": 8, "motion_thr_acc": 9,
    "motion_thr_gyro": 10, "net_ip": 20, "net_mask": 21, "net_gw": 22, "net_dest_ip": 23,
    "stream_port": 24, "stream_when_unbound": 25, "link_port": 26, "line_rate": 40,
}
CFG_NAMES = {v: k for k, v in CFG_IDS.items()}
CFG_TYPES = {  # type used when SETTING (the device echoes its own type in replies)
    1: CFG_U16, 2: CFG_U8, 3: CFG_U8, 4: CFG_U8, 5: CFG_U8, 6: CFG_U8, 7: CFG_U8, 8: CFG_U16,
    9: CFG_F32, 10: CFG_F32, 20: CFG_IP4, 21: CFG_IP4, 22: CFG_IP4, 23: CFG_IP4,
    24: CFG_U16, 25: CFG_U8, 26: CFG_U16, 40: CFG_U16,
}

# ---- struct formats (little-endian, packed) ----------------------------------
HDR = struct.Struct("<HBBHHI")                       # 12
S_HELLO = struct.Struct("<3sBI")                     # 8
S_BIND = struct.Struct("<IHBB4sHHI3s5s")             # 28
S_UNBIND = struct.Struct("<I")                       # 4
S_PING = struct.Struct("<II")                        # 8
S_LED_CMD = struct.Struct("<BBHBBHHBB")              # 12
S_LED_SET_HEAD = struct.Struct("<B3s")               # 4
S_OVL_ITEM = struct.Struct("<14s10sHBB")             # 28
S_OVL_HEAD = struct.Struct("<HBB")                   # 4
S_CFG_ITEM = struct.Struct("<HBBI")                  # 8
S_CFG_HEAD = struct.Struct("<B3s")                   # 4
S_CAL = struct.Struct("<B3s")                        # 4
S_ANNOUNCE = struct.Struct("<12s6sBB3sB16sHHB4s3sIBBBBHHBB4H4HHH14s")  # 100
S_BIND_ACK = struct.Struct("<IB3sHHBBHHHHH8s")       # 32
S_PONG = struct.Struct("<IIIIHBBhB5s")               # 28
S_ERROR = struct.Struct("<BBH")                      # 4
S_LINE_HDR = struct.Struct("<IHHBBH")                # 12
S_HID = struct.Struct("<IHBB4I3f3ff8s")              # 60

SIZES = {  # full datagram sizes as asserted in the C header
    "hdr": 12, "hello": 20, "bind": 40, "unbind": 16, "ping": 20, "led_cmd": 12, "led_set": 64,
    "overlay_item": 28, "oled_overlay": 100, "oled_clear": 12, "cfg_item": 8, "cfg_msg": 144,
    "cal_start": 16, "announce": 112, "bind_ack": 44, "pong": 40, "error": 16, "line_hdr": 24, "hid": 72,
}
assert HDR.size == 12 and HDR.size + S_HELLO.size == 20 and HDR.size + S_BIND.size == 40
assert HDR.size + S_LED_SET_HEAD.size + 4 * S_LED_CMD.size == 64
assert HDR.size + S_OVL_HEAD.size + 3 * S_OVL_ITEM.size == 100
assert HDR.size + S_CFG_HEAD.size + 16 * S_CFG_ITEM.size == 144
assert HDR.size + S_ANNOUNCE.size == 112 and HDR.size + S_BIND_ACK.size == 44
assert HDR.size + S_PONG.size == 40 and HDR.size + S_LINE_HDR.size == 24 and HDR.size + S_HID.size == 72


class Seq:
    def __init__(self):
        self.n = 0

    def next(self):
        v = self.n
        self.n = (self.n + 1) & 0xFFFFFFFF
        return v


def hdr(msg_type: int, length: int, seq: int) -> bytes:
    return HDR.pack(MAGIC, VERSION, msg_type, length, 0, seq)


def parse_hdr(data: bytes):
    """Return (type, length, seq) or None when the datagram is not SLP v1 / truncated."""
    if len(data) < HDR.size:
        return None
    magic, ver, typ, length, _flags, seq = HDR.unpack_from(data)
    if magic != MAGIC or ver != VERSION or length != len(data):
        return None
    return typ, length, seq


# ---- builders (host -> device) ------------------------------------------------
def build_hello(seq, host_version=(0, 0, 0), want=FEAT_ALL):
    body = S_HELLO.pack(bytes(host_version), VERSION, want)
    return hdr(HELLO, HDR.size + len(body), seq) + body


def build_bind(seq, session, stream_port=0, mode=STREAM_UNICAST_TO_SENDER, mcast="0.0.0.0",
               hid_rate=0, dpi=0, want=FEAT_ALL, host_version=(0, 0, 0)):
    body = S_BIND.pack(session, stream_port, mode, 0, ip_bytes(mcast), hid_rate, dpi, want,
                       bytes(host_version), b"\0" * 5)
    return hdr(BIND, HDR.size + len(body), seq) + body


def build_unbind(seq, session):
    body = S_UNBIND.pack(session)
    return hdr(UNBIND, HDR.size + len(body), seq) + body


def build_ping(seq, session, host_time_ms):
    body = S_PING.pack(session, host_time_ms & 0xFFFFFFFF)
    return hdr(PING, HDR.size + len(body), seq) + body


@dataclass
class LedCmd:
    brightness_1: int = 0
    glide_1: int = 0
    time_1_ms: int = 0
    brightness_2: int = 0
    glide_2: int = 0
    time_2_ms: int = 0
    blink_count: int = 0
    flags: int = 0

    def pack(self):
        return S_LED_CMD.pack(self.brightness_1, self.glide_1, self.time_1_ms, self.brightness_2,
                              self.glide_2, self.time_2_ms, self.blink_count, self.flags, 0)

    @classmethod
    def unpack(cls, b):
        f = S_LED_CMD.unpack(b)
        return cls(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7])


def build_led_set(seq, cmds: dict):
    """cmds: {led_index: LedCmd}"""
    mask = 0
    body = b""
    for i in range(MAX_LEDS):
        c = cmds.get(i, LedCmd())
        if i in cmds:
            mask |= 1 << i
        body += c.pack()
    body = S_LED_SET_HEAD.pack(mask, b"\0\0\0") + body
    return hdr(LED_SET, HDR.size + len(body), seq) + body


@dataclass
class OverlayItem:
    label: str = ""
    value: str = ""
    norm: int = 0xFFFF     # 0..65535, 0xFFFF = no bar
    flags: int = 0

    def pack(self):
        return S_OVL_ITEM.pack(self.label.encode("ascii", "replace")[:OVERLAY_LABEL_LEN],
                               self.value.encode("ascii", "replace")[:OVERLAY_VALUE_LEN],
                               self.norm, self.flags, 0)

    @classmethod
    def unpack(cls, b):
        label, value, norm, flags, _ = S_OVL_ITEM.unpack(b)
        return cls(label.split(b"\0")[0].decode("ascii", "replace"),
                   value.split(b"\0")[0].decode("ascii", "replace"), norm, flags)


def build_overlay(seq, items: List[OverlayItem], ttl_ms=1500, layout=0):
    items = items[:OVERLAY_MAX_ITEMS]
    body = S_OVL_HEAD.pack(ttl_ms, len(items), layout)
    for i in range(OVERLAY_MAX_ITEMS):
        body += (items[i] if i < len(items) else OverlayItem()).pack()
    return hdr(OLED_OVERLAY, HDR.size + len(body), seq) + body


def build_oled_clear(seq):
    return hdr(OLED_CLEAR, HDR.size, seq)


def build_cfg(seq, items, set_values=False):
    """items: list of (id, type, value) for SET or list of ids for GET."""
    body = S_CFG_HEAD.pack(min(len(items), CFG_MAX_ITEMS), b"\0\0\0")
    for i in range(CFG_MAX_ITEMS):
        if i < len(items):
            if set_values:
                cid, ctype, val = items[i]
            else:
                cid, ctype, val = items[i], 0, 0
            body += S_CFG_ITEM.pack(cid, ctype, 0, val & 0xFFFFFFFF)
        else:
            body += S_CFG_ITEM.pack(0, 0, 0, 0)
    return hdr(CFG_SET if set_values else CFG_GET, HDR.size + len(body), seq) + body


def build_cal_start(seq, kind):
    body = S_CAL.pack(kind, b"\0\0\0")
    return hdr(CAL_START, HDR.size + len(body), seq) + body


# ---- builders (device -> host), used by the fake device ----------------------
def build_announce(seq, uid=b"\0" * 12, mac=b"\0" * 6, hw_family=1, hw_rev=1, fw=(4, 0, 0), name="Sp3ctra-FAKE",
                   ctrl_port=CTRL_PORT, stream_port=STREAM_PORT, bound=0, peer="0.0.0.0", features=FEAT_ALL,
                   n_buttons=3, n_leds=3, led_kind=0, imu_kind=1, display=(256, 64, 4),
                   dpis=(200, 400), pixels=(1728, 3456), line_rate_max=1000, hid_rate_max=1000):
    d = list(dpis) + [0] * (4 - len(dpis))
    px = list(pixels) + [0] * (4 - len(pixels))
    body = S_ANNOUNCE.pack(uid, mac, hw_family, hw_rev, bytes(fw), VERSION, name.encode()[:NAME_LEN],
                           ctrl_port, stream_port, bound, ip_bytes(peer), b"\0\0\0", features,
                           n_buttons, n_leds, led_kind, imu_kind, display[0], display[1], display[2], len(dpis),
                           *d, *px, line_rate_max, hid_rate_max, b"\0" * 14)
    return hdr(ANNOUNCE, HDR.size + len(body), seq) + body


def build_bind_ack(seq, session, status, dpi=400, pixels=3456, frag_count=12, frag_pixels=288,
                   hid_rate=DEFAULT_HID_RATE_HZ, hid_mask=0x0F, timeout=SESSION_TIMEOUT_MS):
    body = S_BIND_ACK.pack(session, status, b"\0\0\0", dpi, pixels, frag_count, 0, frag_pixels,
                           24 + 3 * frag_pixels, hid_rate, hid_mask, timeout, b"\0" * 8)
    return hdr(BIND_ACK, HDR.size + len(body), seq) + body


def build_pong(seq, session, host_time_ms, uptime_ms, lines_sent, lps, cal_state=8, cal_progress=0,
               temp_c=25.0, link_flags=LINK_STREAMING):
    body = S_PONG.pack(session, host_time_ms & 0xFFFFFFFF, uptime_ms & 0xFFFFFFFF, lines_sent & 0xFFFFFFFF,
                       lps, cal_state, cal_progress, int(temp_c * 10), link_flags, b"\0" * 5)
    return hdr(PONG, HDR.size + len(body), seq) + body


def build_cfg_reply(seq, items):
    """items: list of (id, type, flags, value)"""
    body = S_CFG_HEAD.pack(min(len(items), CFG_MAX_ITEMS), b"\0\0\0")
    for i in range(CFG_MAX_ITEMS):
        cid, ctype, flags, val = items[i] if i < len(items) else (0, 0, 0, 0)
        body += S_CFG_ITEM.pack(cid, ctype, flags, val & 0xFFFFFFFF)
    return hdr(CFG_REPLY, HDR.size + len(body), seq) + body


def build_error(seq, in_reply_to, code):
    body = S_ERROR.pack(in_reply_to, code, 0)
    return hdr(ERROR, HDR.size + len(body), seq) + body


def build_line(seq, line_id, pixel_offset, pixel_count, frag_index, frag_count, r, g, b, period_us=0):
    body = S_LINE_HDR.pack(line_id, pixel_offset, pixel_count, frag_index, frag_count, period_us)
    return hdr(LINE, HDR.size + len(body) + 3 * pixel_count, seq) + body + bytes(r) + bytes(g) + bytes(b)


def build_hid(seq, timestamp_us, button_state, button_seq, acc, gyro, temp_c, valid=0x0F, n_buttons=3):
    bs = list(button_seq) + [0] * (MAX_BUTTONS - len(button_seq))
    body = S_HID.pack(timestamp_us & 0xFFFFFFFF, valid, n_buttons, button_state, *bs[:4], *acc, *gyro, temp_c, b"\0" * 8)
    return hdr(HID, HDR.size + len(body), seq) + body


# ---- parsers ------------------------------------------------------------------
@dataclass
class Announce:
    uid: bytes; mac: bytes; hw_family: int; hw_revision: int; fw_version: tuple; proto_min: int
    name: str; ctrl_port: int; stream_port: int; bound: int; bound_peer_ip: str; features: int
    n_buttons: int; n_leds: int; led_kind: int; imu_kind: int; display_w: int; display_h: int
    display_bpp: int; dpi: list; pixels_at_dpi: list; line_rate_max: int; hid_rate_max: int


def parse_announce(data):
    f = S_ANNOUNCE.unpack_from(data, HDR.size)
    # field order: 0 uid 1 mac 2 hw_family 3 hw_rev 4 fw 5 proto_min 6 name 7 ctrl 8 stream 9 bound
    # 10 peer 11 res0 12 features 13 n_buttons 14 n_leds 15 led_kind 16 imu_kind 17 w 18 h 19 bpp
    # 20 n_dpi 21..24 dpi 25..28 pixels 29 line_rate_max 30 hid_rate_max 31 res1
    n_dpi = f[20]
    return Announce(uid=f[0], mac=f[1], hw_family=f[2], hw_revision=f[3], fw_version=tuple(f[4]), proto_min=f[5],
                    name=f[6].split(b"\0")[0].decode("ascii", "replace"), ctrl_port=f[7], stream_port=f[8],
                    bound=f[9], bound_peer_ip=ip_str(f[10]), features=f[12], n_buttons=f[13], n_leds=f[14],
                    led_kind=f[15], imu_kind=f[16], display_w=f[17], display_h=f[18], display_bpp=f[19],
                    dpi=list(f[21:25])[:n_dpi], pixels_at_dpi=list(f[25:29])[:n_dpi],
                    line_rate_max=f[29], hid_rate_max=f[30])


@dataclass
class BindAck:
    session: int; status: int; dpi: int; pixels_per_line: int; fragment_count: int; fragment_pixels: int
    line_packet_bytes: int; hid_rate_hz: int; hid_valid_mask: int; session_timeout_ms: int


def parse_bind_ack(data):
    f = S_BIND_ACK.unpack_from(data, HDR.size)
    return BindAck(session=f[0], status=f[1], dpi=f[3], pixels_per_line=f[4], fragment_count=f[5],
                   fragment_pixels=f[7], line_packet_bytes=f[8], hid_rate_hz=f[9], hid_valid_mask=f[10],
                   session_timeout_ms=f[11])


@dataclass
class Pong:
    session: int; host_time_ms: int; uptime_ms: int; lines_sent: int; line_rate_lps: int
    cal_state: int; cal_progress: int; temp_c: float; link_flags: int


def parse_pong(data):
    f = S_PONG.unpack_from(data, HDR.size)
    return Pong(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7] / 10.0, f[8])


def parse_cfg_reply(data):
    count = S_CFG_HEAD.unpack_from(data, HDR.size)[0]
    out = []
    off = HDR.size + S_CFG_HEAD.size
    for i in range(min(count, CFG_MAX_ITEMS)):
        out.append(S_CFG_ITEM.unpack_from(data, off + i * S_CFG_ITEM.size))
    return out   # list of (id, type, flags, value)


def parse_error(data):
    in_reply_to, code, _ = S_ERROR.unpack_from(data, HDR.size)
    return in_reply_to, code


@dataclass
class LineFrag:
    seq: int; line_id: int; pixel_offset: int; pixel_count: int; fragment_index: int; fragment_count: int
    period_us: int; r: bytes; g: bytes; b: bytes


def parse_line(data):
    h = parse_hdr(data)
    if not h or h[0] != LINE:
        return None
    line_id, off, n, fi, fc, per = S_LINE_HDR.unpack_from(data, HDR.size)
    base = HDR.size + S_LINE_HDR.size
    if len(data) < base + 3 * n:
        return None
    return LineFrag(h[2], line_id, off, n, fi, fc, per, data[base:base + n], data[base + n:base + 2 * n],
                    data[base + 2 * n:base + 3 * n])


@dataclass
class Hid:
    seq: int; timestamp_us: int; valid_mask: int; button_count: int; button_state: int
    button_seq: list; acc: tuple; gyro: tuple; temp_c: float


def parse_hid(data):
    h = parse_hdr(data)
    if not h or h[0] != HID:
        return None
    f = S_HID.unpack_from(data, HDR.size)
    return Hid(h[2], f[0], f[1], f[2], f[3], list(f[4:8]), tuple(f[8:11]), tuple(f[11:14]), f[14])


# ---- helpers ------------------------------------------------------------------
def ip_bytes(s: str) -> bytes:
    return bytes(int(x) for x in s.split("."))


def ip_str(b) -> str:
    return ".".join(str(x) for x in bytes(b)[:4])


def cfg_pack_value(cid: int, text: str) -> int:
    """Encode a human value for CFG_SET according to the id's type."""
    t = CFG_TYPES.get(cid, CFG_U32)
    if t == CFG_F32:
        return struct.unpack("<I", struct.pack("<f", float(text)))[0]
    if t == CFG_IP4:
        b = ip_bytes(text)
        return b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)
    return int(text, 0) & 0xFFFFFFFF


def cfg_format_value(ctype: int, value: int) -> str:
    if ctype == CFG_F32:
        return "%g" % struct.unpack("<f", struct.pack("<I", value))[0]
    if ctype == CFG_IP4:
        return "%d.%d.%d.%d" % (value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF)
    return str(value)


def cfg_flags_str(flags: int) -> str:
    names = []
    if flags & CFG_F_REBOOT: names.append("reboot")
    if flags & CFG_F_READONLY: names.append("read-only")
    if flags & CFG_F_UNKNOWN: names.append("unknown")
    if flags & CFG_F_REJECTED: names.append("rejected")
    return ",".join(names)


# ---- self-test ------------------------------------------------------------------
if __name__ == "__main__":
    q = Seq()
    a = parse_announce(build_announce(q.next(), b"\x01" * 12, b"\x02\x53\x33\x04\x05\x06", name="Sp3ctra-TEST",
                                      bound=1, peer="192.168.100.20", dpis=(200, 400), pixels=(1728, 3456)))
    assert a.name == "Sp3ctra-TEST" and a.bound == 1 and a.bound_peer_ip == "192.168.100.20"
    assert a.dpi == [200, 400] and a.pixels_at_dpi == [1728, 3456] and a.display_bpp == 4 and a.hid_rate_max == 1000
    ack = parse_bind_ack(build_bind_ack(q.next(), 0xDEADBEEF, BIND_OK, 400, 3456, 12, 288, 200))
    assert ack.session == 0xDEADBEEF and ack.fragment_count == 12 and ack.fragment_pixels == 288 and ack.line_packet_bytes == 888
    pg = parse_pong(build_pong(q.next(), 7, 1234, 5000, 42, 400, temp_c=31.5))
    assert pg.session == 7 and pg.host_time_ms == 1234 and pg.temp_c == 31.5 and pg.line_rate_lps == 400
    h = parse_hid(build_hid(q.next(), 99, 0b101, [1, 2, 3], (0.1, 0.2, 1.0), (1.5, -2.5, 0.0), 30.0))
    assert h.button_state == 5 and h.button_seq[:3] == [1, 2, 3] and abs(h.acc[2] - 1.0) < 1e-6 and abs(h.gyro[1] + 2.5) < 1e-6
    ln = parse_line(build_line(q.next(), 5, 288, 288, 1, 12, bytes(range(256)) + bytes(32), bytes(288), bytes(288), 2500))
    assert ln.line_id == 5 and ln.pixel_offset == 288 and ln.fragment_index == 1 and ln.r[255] == 255 and len(ln.b) == 288
    c = LedCmd.unpack(LedCmd(100, 10, 200, 0, 0, 300, 3, LED_NO_LOCAL_PRESS).pack())
    assert c.brightness_1 == 100 and c.time_2_ms == 300 and c.blink_count == 3 and c.flags == 1
    it = OverlayItem.unpack(OverlayItem("Speed", "12 ms", 32768, OVL_BIPOLAR).pack())
    assert it.label == "Speed" and it.value == "12 ms" and it.norm == 32768
    r = parse_cfg_reply(build_cfg_reply(q.next(), [(CFG_IDS["dpi"], CFG_U16, CFG_F_REBOOT, 400)]))
    assert r[0] == (1, CFG_U16, CFG_F_REBOOT, 400)
    assert cfg_format_value(CFG_IP4, cfg_pack_value(CFG_IDS["net_ip"], "192.168.100.1")) == "192.168.100.1"
    assert abs(float(cfg_format_value(CFG_F32, cfg_pack_value(CFG_IDS["motion_thr_acc"], "0.25"))) - 0.25) < 1e-6
    for name, size in SIZES.items():
        pass
    assert len(build_hello(0)) == SIZES["hello"] and len(build_bind(0, 1)) == SIZES["bind"]
    assert len(build_led_set(0, {})) == SIZES["led_set"] and len(build_overlay(0, [])) == SIZES["oled_overlay"]
    assert len(build_cfg(0, [1])) == SIZES["cfg_msg"] and len(build_cal_start(0, 0)) == SIZES["cal_start"]
    assert len(build_error(0, 1, 2)) == SIZES["error"] and len(build_ping(0, 1, 2)) == SIZES["ping"]
    print("slp.py self-test OK")
