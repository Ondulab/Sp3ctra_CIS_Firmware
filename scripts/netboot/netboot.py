#!/usr/bin/env python3
"""Client du flasheur reseau du bootloader Sp3ctra (protocole SNB, UDP 55152).

Contrat de fil : Common/Inc/netboot_protocol.h. Une requete par datagramme,
stop-and-wait avec retransmission sur silence ; les numeros de sequence
ecartent les reponses en retard.

    from netboot import NetBoot
    nb = NetBoot("192.168.100.1")
    info = nb.info()
    nb.flash_image(0x08100000, open("fw.bin", "rb").read())
    nb.boot()

Note macOS : le terminal doit avoir l'autorisation "Reseau local".
"""

import errno
import os
import socket
import struct
import sys
import time
import zlib

PORT = 55152
MAGIC = 0x31424E53          # "SNB1"
PROTO_VERSION = 1
MAX_DATA = 1024
SECTOR = 0x20000
FLASH_START = 0x08000000
FLASH_END = 0x08200000
BL_SLOTS = {0: 0x08000000, 1: 0x080E0000}   # bootloader slot A / B (BOOT_ADD0)

# Requetes
DISCOVER, ERASE, WRITE, READ, CRC, LOG, JOURNAL, BOOT, PING, BOOTSEL = range(1, 11)
# Reponses
INFO, ACK, DATA, CRC_REPLY, LOG_REPLY = 0x81, 0x82, 0x84, 0x85, 0x86
FLAG_RELEASE = 0x01

STATUS = {0: "OK", 1: "bad argument", 2: "address out of range", 3: "flash error",
          4: "busy (another host holds the session)", 5: "verify mismatch",
          6: "target not erased", 7: "unknown request"}
REASON = {0: "none", 1: "requested by the application", 2: "buttons held at power-on",
          3: "OTA journal in FAILED state"}
PHASE = {0: "-", 0x11: "IDLE", 0x22: "PENDING", 0x33: "TRIAL", 0x44: "ROLLBACK", 0x55: "FAILED"}
PHASE_BY_NAME = {v.lower(): k for k, v in PHASE.items() if k}
PHASE_BY_NAME["clear"] = 0

HDR = struct.Struct("<IBBH")
INFO_S = struct.Struct("<BBH6s4s12s16s16sBBBBIIBB2s")
RANGE_S = struct.Struct("<II")
WRITE_S = struct.Struct("<IHH")
READ_S = struct.Struct("<IHH")
ACK_S = struct.Struct("<B3sII")
CRC_S = struct.Struct("<III")
LOGREQ_S = struct.Struct("<IHH")
LOGREP_S = struct.Struct("<IHH")
JOURNAL_S = struct.Struct("<BBBB")
BOOT_S = struct.Struct("<B3s")
BOOTSEL_S = struct.Struct("<I")

# Adresses des images (Common/Inc/boot_config.h)
IMAGES = {
    "cm7": 0x08100000,          # slot A, le seul demarre aujourd'hui
    "cm4": 0x08040000,
    "cm7-b": 0x08180000,
    "cm4-b": 0x08060000,
    "bl-a": 0x08000000,         # bootloader, slot A
    "bl-b": 0x080E0000,         # bootloader, slot B (BOOT_ADD0)
    "journal": 0x08020000,
}


class NetBootError(Exception):
    pass


class Timeout(NetBootError):
    pass


class DeviceInfo:
    def __init__(self, addr, raw):
        (self.proto, self.state, self.max_data, mac, ip, uid, name, blv, self.journal_phase,
         self.trial, self.rollback, self.pending, self.reset_flags, self.uptime_ms,
         self.reason, self.boot_slot, _) = INFO_S.unpack(raw[:INFO_S.size])
        self.addr = addr
        self.mac = ":".join("%02X" % b for b in mac)
        self.ip = ".".join(str(b) for b in ip)
        self.uid = uid.hex()
        self.name = name.split(b"\0", 1)[0].decode("ascii", "replace")
        self.bl_version = blv.split(b"\0", 1)[0].decode("ascii", "replace")

    def __str__(self):
        rst = []
        for bit, lbl in ((23, "POR"), (21, "BOR"), (22, "PIN"), (24, "SOFT"), (26, "IWDG"), (28, "WWDG")):
            if self.reset_flags & (1 << bit):
                rst.append(lbl)
        return ("%s  ip %s  mac %s  bl %s (slot %s)  %s  journal %s (trial %d, rollback %d, pending %d)"
                "  reset %s  up %.1fs  reason: %s" % (
                    self.name, self.ip, self.mac, self.bl_version, "B" if self.boot_slot else "A",
                    "BUSY" if self.state else "free", PHASE.get(self.journal_phase, hex(self.journal_phase)),
                    self.trial, self.rollback, self.pending, "+".join(rst) or "-",
                    self.uptime_ms / 1000.0, REASON.get(self.reason, str(self.reason))))


def sectors_covering(addr, length):
    """Secteurs de 128 Ko (adresse de base) recouvrant [addr, addr+length)."""
    first = addr & ~(SECTOR - 1)
    last = (addr + length - 1) & ~(SECTOR - 1)
    return list(range(first, last + 1, SECTOR))


def pad32(data):
    rem = len(data) % 32
    return data if rem == 0 else data + b"\xff" * (32 - rem)


class NetBoot:
    def __init__(self, host=None, timeout=1.0, retries=5, port=PORT, verbose=False):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.retries = retries
        self.verbose = verbose
        self.seq = int(time.time() * 1000) & 0xFFFF
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.bind(("0.0.0.0", 0))
        self.max_data = MAX_DATA
        self.boot_slot = None          # slot du bootloader qui tourne, connu apres info()

    def close(self):
        self.sock.close()

    def release_at_exit(self):
        """Libere la session quand le processus se termine, quoi qu'il arrive."""
        import atexit
        atexit.register(self.release)
        return self

    # ---- transport -------------------------------------------------------

    def _next_seq(self):
        self.seq = (self.seq + 1) & 0xFFFF
        return self.seq

    def _send(self, dest, rtype, payload, seq, flags=0):
        try:
            self.sock.sendto(HDR.pack(MAGIC, rtype, flags, seq) + payload, (dest, self.port))
        except OSError as e:
            if e.errno in (errno.EHOSTUNREACH, errno.ENETUNREACH, errno.EACCES, errno.EPERM):
                raise NetBootError(
                    "cannot send to %s: %s.\n"
                    "On macOS this is the 'Local Network' permission of the terminal you are using\n"
                    "(System Settings > Privacy & Security > Local Network: enable it for that app,\n"
                    "then relaunch the app -- the verdict is cached per process), or a VPN that\n"
                    "blocks the LAN. Terminal.app is known to work: scripts/ota/relay.sh delegates to it."
                    % (dest, e.strerror))
            raise

    def _recv(self, deadline):
        while True:
            left = deadline - time.time()
            if left <= 0:
                return None
            self.sock.settimeout(left)
            try:
                data, addr = self.sock.recvfrom(2048)
            except socket.timeout:
                return None
            if len(data) < HDR.size:
                continue
            magic, rtype, _flags, seq = HDR.unpack(data[:HDR.size])
            if magic != MAGIC:
                continue
            return rtype, seq, data[HDR.size:], addr

    def request(self, rtype, payload=b"", timeout=None, retries=None, dest=None, flags=0):
        """Stop-and-wait : renvoie (type, charge utile) ou leve Timeout."""
        dest = dest or self.host
        if dest is None:
            raise NetBootError("no host: pass --host or run discover")
        timeout = self.timeout if timeout is None else timeout
        retries = self.retries if retries is None else retries
        seq = self._next_seq()
        for attempt in range(retries + 1):
            self._send(dest, rtype, payload, seq, flags)
            deadline = time.time() + timeout
            while True:
                r = self._recv(deadline)
                if r is None:
                    break
                rt, rseq, body, _ = r
                if rseq != seq:
                    continue                # reponse en retard d'une requete precedente
                return rt, body
            if self.verbose:
                print("  retry %d/%d (type 0x%02x)" % (attempt + 1, retries, rtype), file=sys.stderr)
        raise Timeout("no answer from %s for request 0x%02x" % (dest, rtype))

    def _ack(self, rtype, payload, **kw):
        rt, body = self.request(rtype, payload, **kw)
        if rt != ACK or len(body) < ACK_S.size:
            raise NetBootError("unexpected reply 0x%02x" % rt)
        status, _, code, elapsed = ACK_S.unpack(body[:ACK_S.size])
        if status != 0:
            raise NetBootError("%s (code 0x%08X)" % (STATUS.get(status, status), code))
        return code, elapsed

    # ---- commandes -------------------------------------------------------

    def discover(self, broadcast="255.255.255.255", wait=1.0):
        """DISCOVER en broadcast (ou unicast si broadcast est une adresse hote).
        Retourne la liste des DeviceInfo qui ont repondu pendant `wait`."""
        seq = self._next_seq()
        self._send(broadcast, DISCOVER, b"", seq)
        found = {}
        deadline = time.time() + wait
        while True:
            r = self._recv(deadline)
            if r is None:
                break
            rt, rseq, body, addr = r
            if rt == INFO and rseq == seq and len(body) >= INFO_S.size:
                found[addr[0]] = DeviceInfo(addr[0], body)
        return list(found.values())

    def info(self, timeout=None):
        rt, body = self.request(DISCOVER, timeout=timeout)
        if rt != INFO:
            raise NetBootError("unexpected reply 0x%02x" % rt)
        i = DeviceInfo(self.host, body)
        self.max_data = i.max_data or MAX_DATA
        self.boot_slot = i.boot_slot
        return i

    def ping(self):
        self._ack(PING, b"")

    def release(self):
        """Libere la session sans attendre son expiration (fin d'une invocation)."""
        try:
            self._ack(PING, b"", retries=0, timeout=0.3, flags=FLAG_RELEASE)
        except (NetBootError, OSError):
            pass

    def erase(self, addr, length, timeout=8.0):
        """Efface les secteurs recouvrant [addr, addr+length). Retourne les ms mesurees."""
        _code, elapsed = self._ack(ERASE, RANGE_S.pack(addr, length), timeout=timeout, retries=1)
        return elapsed

    def write(self, addr, data, progress=None):
        data = pad32(data)
        if addr % 32:
            raise NetBootError("address must be 32-byte aligned")
        off = 0
        while off < len(data):
            chunk = data[off:off + self.max_data]
            self._ack(WRITE, WRITE_S.pack(addr + off, len(chunk), 0) + chunk)
            off += len(chunk)
            if progress:
                progress(off, len(data))
        return len(data)

    def read(self, addr, length, progress=None):
        out = bytearray()
        while len(out) < length:
            n = min(self.max_data, length - len(out))
            rt, body = self.request(READ, READ_S.pack(addr + len(out), n, 0))
            if rt == ACK:
                status, _, code, _ = ACK_S.unpack(body[:ACK_S.size])
                raise NetBootError("%s (code 0x%08X)" % (STATUS.get(status, status), code))
            if rt != DATA:
                raise NetBootError("unexpected reply 0x%02x" % rt)
            _a, ln, _ = WRITE_S.unpack(body[:WRITE_S.size])
            out += body[WRITE_S.size:WRITE_S.size + ln]
            if progress:
                progress(len(out), length)
        return bytes(out)

    def crc(self, addr, length, timeout=3.0):
        rt, body = self.request(CRC, RANGE_S.pack(addr, length), timeout=timeout)
        if rt == ACK:
            status, _, code, _ = ACK_S.unpack(body[:ACK_S.size])
            raise NetBootError("%s (code 0x%08X)" % (STATUS.get(status, status), code))
        if rt != CRC_REPLY:
            raise NetBootError("unexpected reply 0x%02x" % rt)
        _a, _l, crc = CRC_S.unpack(body[:CRC_S.size])
        return crc

    def log(self, since=0, maxlen=MAX_DATA, src=0):
        """Retourne (next, texte) depuis la position `since` de l'anneau `src`."""
        rt, body = self.request(LOG, LOGREQ_S.pack(since, min(maxlen, self.max_data), src))
        if rt != LOG_REPLY:
            raise NetBootError("unexpected reply 0x%02x" % rt)
        nxt, ln, _ = LOGREP_S.unpack(body[:LOGREP_S.size])
        return nxt, body[LOGREP_S.size:LOGREP_S.size + ln].decode("utf-8", "replace")

    def log_all(self, src=0, since=0):
        """Draine l'anneau depuis `since` (0 = tout ce qui reste). Retourne (next, texte)."""
        parts = []
        while True:
            nxt, text = self.log(since, src=src)
            if not text:
                return nxt, "".join(parts)
            parts.append(text)
            since = nxt

    def journal(self, phase, trial=0, rollback=0, pending=0):
        self._ack(JOURNAL, JOURNAL_S.pack(phase, trial, rollback, pending))

    def boot(self):
        self._ack(BOOT, BOOT_S.pack(0, b"\0\0\0"), retries=0)

    def boot_select(self, addr):
        """Programme BOOT_ADD0 vers un slot du bootloader (prend effet au reset)."""
        self._ack(BOOTSEL, BOOTSEL_S.pack(addr), timeout=3.0, retries=1)

    # ---- sequence complete ----------------------------------------------

    def flash_image(self, addr, data, log=print, verify=True, erase=True):
        """Efface, ecrit, verifie. Retourne un dict de durees."""
        data = pad32(data)
        if addr < FLASH_START or addr + len(data) > FLASH_END:
            raise NetBootError("image does not fit in the flash")
        if self.boot_slot is not None:
            running = BL_SLOTS[self.boot_slot]
            if addr < running + SECTOR and addr + len(data) > running:
                raise NetBootError("that range holds the running bootloader (slot %s); flash the other slot"
                                   % ("B" if self.boot_slot else "A"))
        t = {}
        if erase:
            t0 = time.time()
            secs = sectors_covering(addr, len(data))
            for i, s in enumerate(secs):
                ms = self.erase(s, SECTOR)
                log("  erased 0x%08X (%d/%d) in %.2f s" % (s, i + 1, len(secs), ms / 1000.0))
            t["erase"] = time.time() - t0
        t0 = time.time()
        last = [0]

        def prog(done, total):
            pct = done * 100 // total
            if pct >= last[0] + 10 or done == total:
                last[0] = pct
                log("  written %6d / %d bytes (%d %%)" % (done, total, pct))
        self.write(addr, data, prog)
        t["write"] = time.time() - t0
        if verify:
            t0 = time.time()
            want = zlib.crc32(data) & 0xFFFFFFFF
            got = self.crc(addr, len(data))
            t["verify"] = time.time() - t0
            if got != want:
                raise NetBootError("CRC mismatch after write: device 0x%08X, file 0x%08X" % (got, want))
            log("  verified: CRC-32 0x%08X over %d bytes" % (got, len(data)))
        return t


def image_bytes(path):
    """Contenu binaire d'un .bin, ou d'un .elf converti par objcopy."""
    if path.lower().endswith(".elf"):
        import shutil
        import subprocess
        import tempfile
        objcopy = shutil.which("arm-none-eabi-objcopy")
        if objcopy is None:
            import glob
            cands = glob.glob("/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins/"
                              "com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin/arm-none-eabi-objcopy")
            objcopy = cands[-1] if cands else None
        if objcopy is None:
            raise NetBootError("arm-none-eabi-objcopy not found: pass a .bin")
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as tmp:
            out = tmp.name
        try:
            subprocess.check_call([objcopy, "-O", "binary", path, out])
            with open(out, "rb") as f:
                return f.read()
        finally:
            os.unlink(out)
    with open(path, "rb") as f:
        return f.read()
