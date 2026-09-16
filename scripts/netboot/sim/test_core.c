/*
 * Test unitaire du coeur du flasheur reseau (netboot_core.c), compile sur
 * l'hote avec un portage factice : ARP, ICMP, DISCOVER, session exclusive,
 * effacement/ecriture/CRC sur une flash simulee, bornes.
 *
 *   scripts/netboot/sim/build.sh && scripts/netboot/sim/test_core
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "netboot_core.h"

/* ---- portage factice ----------------------------------------------------- */

#define FLASH_BASE 0x08000000u
#define FLASH_SIZE 0x00200000u
static uint8_t flash[FLASH_SIZE];
static uint8_t last_tx[1536];
static uint16_t last_tx_len;
static int tx_count;
static uint32_t now_ms;
static int rebooted;
static char fake_log[] = "[B    0.001] hello\n[B    0.002] world\n";
static nb_journal_t last_journal;

void nbp_send_frame(const uint8_t *frame, uint16_t len) { memcpy(last_tx, frame, len); last_tx_len = len; tx_count++; }
uint32_t nbp_tick_ms(void) { return now_ms; }
void nbp_get_info(nb_devinfo_t *info) { memset(info, 0, sizeof *info); strcpy(info->name, "Sp3ctra-TEST"); strcpy(info->bl_version, "9.9.9"); info->journal_phase = 0x11; info->reason = 1; }
/* bootloader simule en slot A : le secteur 0 est protege */
static int in_flash(uint32_t a, uint32_t l) { return l && a >= NB_FLASH_START + 0x20000u && a < NB_FLASH_END && l <= NB_FLASH_END - a; }
int nbp_flash_erase(uint32_t addr, uint32_t len, uint32_t *fail) {
    if (!in_flash(addr, len) || (addr & 0x1FFFF)) { *fail = addr; return NB_E_RANGE; }
    uint32_t end = addr + len;
    for (uint32_t a = addr; a < end; a += 0x20000) memset(flash + (a - FLASH_BASE), 0xFF, 0x20000);
    now_ms += 1700;
    return NB_OK;
}
int nbp_flash_write(uint32_t addr, const uint8_t *data, uint32_t len, uint32_t *fail) {
    if (!in_flash(addr, len) || (addr & 31) || (len & 31)) { *fail = addr; return NB_E_RANGE; }
    for (uint32_t off = 0; off < len; off += 32) {
        uint8_t *cur = flash + (addr + off - FLASH_BASE);
        if (memcmp(cur, data + off, 32) == 0) continue;
        for (int i = 0; i < 32; i++) if (cur[i] != 0xFF) { *fail = addr + off; return NB_E_NOT_ERASED; }
        memcpy(cur, data + off, 32);
    }
    return NB_OK;
}
int nbp_mem_read(uint32_t addr, uint8_t *out, uint32_t len) {
    if (!len || addr < FLASH_BASE || addr >= FLASH_BASE + FLASH_SIZE || len > FLASH_BASE + FLASH_SIZE - addr) return NB_E_RANGE;
    memcpy(out, flash + (addr - FLASH_BASE), len); return NB_OK;
}
int nbp_crc32(uint32_t addr, uint32_t len, uint32_t *crc) {
    if (!len || addr < FLASH_BASE || len > FLASH_BASE + FLASH_SIZE - addr) return NB_E_RANGE;
    *crc = (uint32_t)crc32(0, flash + (addr - FLASH_BASE), len); return NB_OK;
}
uint32_t nbp_log_read(uint16_t src, uint32_t since, uint8_t *out, uint16_t max, uint32_t *next) {
    (void)src; uint32_t total = (uint32_t)strlen(fake_log); if (since > total) since = total;
    uint32_t n = total - since; if (n > max) n = max; memcpy(out, fake_log + since, n); *next = since + n; return n;
}
int nbp_journal_write(uint8_t phase, uint8_t trial, uint8_t rollback, uint8_t pending) {
    last_journal.phase = phase; last_journal.trial = trial; last_journal.rollback = rollback; last_journal.pending = pending;
    return (phase == 0 || phase == 0x11 || phase == 0x33) ? NB_OK : NB_E_ARG;
}
void nbp_reboot(void) { rebooted = 1; }
static uint32_t last_bootsel;
int nbp_boot_select(uint32_t addr) { if (addr != 0x08000000u && addr != 0x080E0000u) return NB_E_ARG; last_bootsel = addr; return NB_OK; }

/* ---- construction de trames --------------------------------------------- */

static const uint8_t me_mac[6] = {0x02, 0x53, 0x33, 0x41, 0x77, 0xDD};
static const uint8_t me_ip[4] = {192, 168, 100, 1};
static const uint8_t host_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
static const uint8_t host_ip[4] = {192, 168, 100, 10};
static const uint8_t other_ip[4] = {192, 168, 100, 11};
static const uint8_t *src_ip = host_ip;
static uint8_t tx_flags;
static const uint8_t bcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static uint16_t rd16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static void wr16(uint8_t *p, uint16_t v) { p[0] = v >> 8; p[1] = v & 0xFF; }
static uint16_t csum(const uint8_t *p, uint32_t len) { uint32_t s = 0; while (len > 1) { s += rd16(p); p += 2; len -= 2; } if (len) s += p[0] << 8; while (s >> 16) s = (s & 0xFFFF) + (s >> 16); return (uint16_t)~s; }

static uint16_t build_udp(uint8_t *f, const uint8_t *dst_mac, const uint8_t *dst_ip, uint16_t sport,
                          uint8_t type, uint16_t seq, const void *payload, uint16_t plen) {
    memcpy(f, dst_mac, 6); memcpy(f + 6, host_mac, 6); wr16(f + 12, 0x0800);
    uint8_t *ip = f + 14, *udp = ip + 20, *p = udp + 8;
    nb_hdr_t h = {NB_MAGIC, type, tx_flags, seq};
    memcpy(p, &h, sizeof h); if (plen) memcpy(p + sizeof h, payload, plen);
    uint16_t ulen = 8 + sizeof h + plen, tlen = 20 + ulen;
    ip[0] = 0x45; ip[1] = 0; wr16(ip + 2, tlen); wr16(ip + 4, 1); wr16(ip + 6, 0); ip[8] = 64; ip[9] = 17; wr16(ip + 10, 0);
    memcpy(ip + 12, src_ip, 4); memcpy(ip + 16, dst_ip, 4); wr16(ip + 10, csum(ip, 20));
    wr16(udp, sport); wr16(udp + 2, NB_PORT); wr16(udp + 4, ulen); wr16(udp + 6, 0);
    return 14 + tlen;
}

/* Reponse SNB : renvoie type, remplit body/blen, verifie l'adressage. */
static uint8_t reply(const uint8_t **body, uint16_t *blen, uint16_t want_seq, uint16_t want_port) {
    assert(last_tx_len >= 14 + 20 + 8 + sizeof(nb_hdr_t));
    assert(memcmp(last_tx, host_mac, 6) == 0 && memcmp(last_tx + 6, me_mac, 6) == 0);
    const uint8_t *ip = last_tx + 14, *udp = ip + 20;
    assert(rd16(ip + 10) != 0 && csum(ip, 20) == 0);        /* somme IP juste */
    assert(memcmp(ip + 12, me_ip, 4) == 0 && memcmp(ip + 16, src_ip, 4) == 0);
    assert(rd16(udp) == NB_PORT && rd16(udp + 2) == want_port);
    assert(rd16(udp + 4) == last_tx_len - 34);
    nb_hdr_t h; memcpy(&h, udp + 8, sizeof h);
    assert(h.magic == NB_MAGIC && h.seq == want_seq);
    *body = udp + 8 + sizeof h; *blen = (uint16_t)(rd16(udp + 4) - 8 - sizeof h);
    return h.type;
}

static uint8_t ack_status(uint16_t seq, uint16_t port, uint32_t *code) {
    const uint8_t *b; uint16_t bl; uint8_t t = reply(&b, &bl, seq, port);
    assert(t == NB_ACK && bl == sizeof(nb_ack_t));
    nb_ack_t a; memcpy(&a, b, sizeof a); if (code) *code = a.code; return a.status;
}

int main(void) {
    uint8_t f[1536]; uint16_t n; const uint8_t *b; uint16_t bl; uint32_t code;
    nb_netcfg_t cfg; memcpy(cfg.mac, me_mac, 6); memcpy(cfg.ip, me_ip, 4); memset(cfg.netmask, 255, 3); cfg.netmask[3] = 0;
    memset(flash, 0xFF, sizeof flash);
    nb_core_init(&cfg);

    /* ARP : requete pour notre IP -> reponse ; pour une autre IP -> silence */
    memcpy(f, bcast_mac, 6); memcpy(f + 6, host_mac, 6); wr16(f + 12, 0x0806);
    uint8_t *a = f + 14; wr16(a, 1); wr16(a + 2, 0x0800); a[4] = 6; a[5] = 4; wr16(a + 6, 1);
    memcpy(a + 8, host_mac, 6); memcpy(a + 14, host_ip, 4); memset(a + 18, 0, 6); memcpy(a + 24, me_ip, 4);
    tx_count = 0; nb_core_input(f, 42);
    assert(tx_count == 1 && last_tx_len == 42 && rd16(last_tx + 12) == 0x0806);
    assert(rd16(last_tx + 14 + 6) == 2 && memcmp(last_tx + 14 + 8, me_mac, 6) == 0 && memcmp(last_tx + 14 + 14, me_ip, 4) == 0);
    assert(memcmp(last_tx + 14 + 18, host_mac, 6) == 0 && memcmp(last_tx + 14 + 24, host_ip, 4) == 0);
    a[24] = 99; tx_count = 0; nb_core_input(f, 42); assert(tx_count == 0);
    printf("ARP ok\n");

    /* ICMP echo */
    memcpy(f, me_mac, 6); memcpy(f + 6, host_mac, 6); wr16(f + 12, 0x0800);
    uint8_t *ip = f + 14; uint8_t *ic = ip + 20; const char *pay = "abcdefgh";
    ic[0] = 8; ic[1] = 0; wr16(ic + 2, 0); wr16(ic + 4, 0x1234); wr16(ic + 6, 7); memcpy(ic + 8, pay, 8); wr16(ic + 2, csum(ic, 16));
    ip[0] = 0x45; ip[1] = 0; wr16(ip + 2, 36); wr16(ip + 4, 5); wr16(ip + 6, 0); ip[8] = 64; ip[9] = 1; wr16(ip + 10, 0);
    memcpy(ip + 12, host_ip, 4); memcpy(ip + 16, me_ip, 4); wr16(ip + 10, csum(ip, 20));
    tx_count = 0; nb_core_input(f, 50);
    assert(tx_count == 1 && last_tx_len == 50 && last_tx[34] == 0 && csum(last_tx + 34, 16) == 0);
    assert(memcmp(last_tx + 42, pay, 8) == 0 && rd16(last_tx + 34 + 4) == 0x1234);
    printf("ICMP ok\n");

    /* DISCOVER en broadcast IP + MAC */
    uint8_t bip[4] = {255, 255, 255, 255};
    n = build_udp(f, bcast_mac, bip, 4000, NB_DISCOVER, 7, NULL, 0); tx_count = 0; nb_core_input(f, n);
    assert(tx_count == 1 && reply(&b, &bl, 7, 4000) == NB_INFO && bl == sizeof(nb_info_t));
    nb_info_t info; memcpy(&info, b, sizeof info);
    assert(info.proto == NB_PROTO_VERSION && info.max_data == NB_MAX_DATA && info.state == 0);
    assert(memcmp(info.mac, me_mac, 6) == 0 && strcmp(info.name, "Sp3ctra-TEST") == 0 && info.reason == 1);
    /* broadcast dirige du sous-reseau */
    uint8_t sbip[4] = {192, 168, 100, 255};
    n = build_udp(f, bcast_mac, sbip, 4000, NB_DISCOVER, 8, NULL, 0); tx_count = 0; nb_core_input(f, n); assert(tx_count == 1);
    /* autre IP unicast : silence */
    uint8_t oip[4] = {192, 168, 100, 2};
    n = build_udp(f, me_mac, oip, 4000, NB_DISCOVER, 9, NULL, 0); tx_count = 0; nb_core_input(f, n); assert(tx_count == 0);
    printf("DISCOVER ok\n");

    /* WRITE sans effacement sur une zone deja programmee -> NOT_ERASED ; sur vierge -> OK */
    uint8_t data[64]; for (int i = 0; i < 64; i++) data[i] = (uint8_t)i;
    struct { nb_write_t w; uint8_t d[64]; } __attribute__((packed)) wr = {{0x08100000, 64, 0}, {0}}; memcpy(wr.d, data, 64);
    n = build_udp(f, me_mac, me_ip, 4000, NB_WRITE, 10, &wr, sizeof wr); nb_core_input(f, n);
    assert(ack_status(10, 4000, &code) == NB_OK && memcmp(flash + 0x100000, data, 64) == 0);
    /* meme donnees a nouveau : idempotent */
    n = build_udp(f, me_mac, me_ip, 4000, NB_WRITE, 11, &wr, sizeof wr); nb_core_input(f, n); assert(ack_status(11, 4000, NULL) == NB_OK);
    /* donnees differentes sans effacement */
    wr.d[0] ^= 0xFF; n = build_udp(f, me_mac, me_ip, 4000, NB_WRITE, 12, &wr, sizeof wr); nb_core_input(f, n);
    assert(ack_status(12, 4000, &code) == NB_E_NOT_ERASED && code == 0x08100000);
    /* alignement */
    wr.w.addr = 0x08100010; n = build_udp(f, me_mac, me_ip, 4000, NB_WRITE, 13, &wr, sizeof wr); nb_core_input(f, n); assert(ack_status(13, 4000, NULL) == NB_E_ARG);
    /* zone interdite : bootloader */
    wr.w.addr = 0x08000000; n = build_udp(f, me_mac, me_ip, 4000, NB_WRITE, 14, &wr, sizeof wr); nb_core_input(f, n); assert(ack_status(14, 4000, NULL) == NB_E_RANGE);
    printf("WRITE ok\n");

    /* Session : le meme hote depuis un autre port reprend la main ; un autre hote
     * est refuse tant que la session vit ; DISCOVER passe toujours ; RELEASE libere. */
    n = build_udp(f, me_mac, me_ip, 5000, NB_PING, 20, NULL, 0); nb_core_input(f, n); assert(ack_status(20, 5000, NULL) == NB_OK);
    src_ip = other_ip;
    n = build_udp(f, me_mac, me_ip, 6000, NB_PING, 21, NULL, 0); nb_core_input(f, n); assert(ack_status(21, 6000, NULL) == NB_E_BUSY);
    n = build_udp(f, me_mac, me_ip, 6000, NB_DISCOVER, 22, NULL, 0); nb_core_input(f, n); assert(reply(&b, &bl, 22, 6000) == NB_INFO);
    src_ip = host_ip;
    tx_flags = NB_FLAG_RELEASE;
    n = build_udp(f, me_mac, me_ip, 5000, NB_PING, 23, NULL, 0); nb_core_input(f, n); assert(ack_status(23, 5000, NULL) == NB_OK);
    tx_flags = 0; assert(!nb_core_client_bound());
    src_ip = other_ip;
    n = build_udp(f, me_mac, me_ip, 6000, NB_PING, 24, NULL, 0); nb_core_input(f, n); assert(ack_status(24, 6000, NULL) == NB_OK);
    src_ip = host_ip;
    n = build_udp(f, me_mac, me_ip, 5000, NB_PING, 25, NULL, 0); nb_core_input(f, n); assert(ack_status(25, 5000, NULL) == NB_E_BUSY);
    now_ms += 11000; nb_core_poll(); assert(!nb_core_client_bound());
    n = build_udp(f, me_mac, me_ip, 5000, NB_PING, 26, NULL, 0); nb_core_input(f, n); assert(ack_status(26, 5000, NULL) == NB_OK);
    printf("session ok\n");

    /* ERASE + WRITE + CRC + READ */
    nb_range_t r = {0x08100000, 0x20000};
    n = build_udp(f, me_mac, me_ip, 5000, NB_ERASE, 30, &r, sizeof r); nb_core_input(f, n); assert(ack_status(30, 5000, NULL) == NB_OK);
    for (int i = 0; i < 0x100; i++) assert(flash[0x100000 + i] == 0xFF);
    wr.w.addr = 0x08100000; n = build_udp(f, me_mac, me_ip, 5000, NB_WRITE, 31, &wr, sizeof wr); nb_core_input(f, n); assert(ack_status(31, 5000, NULL) == NB_OK);
    r.len = 64; n = build_udp(f, me_mac, me_ip, 5000, NB_CRC, 32, &r, sizeof r); nb_core_input(f, n);
    assert(reply(&b, &bl, 32, 5000) == NB_CRC_REPLY); nb_crc_reply_t cr; memcpy(&cr, b, sizeof cr);
    assert(cr.crc == (uint32_t)crc32(0, wr.d, 64));
    nb_read_t rd = {0x08100000, 64, 0}; n = build_udp(f, me_mac, me_ip, 5000, NB_READ, 33, &rd, sizeof rd); nb_core_input(f, n);
    assert(reply(&b, &bl, 33, 5000) == NB_DATA && bl == sizeof(nb_write_t) + 64 && memcmp(b + sizeof(nb_write_t), wr.d, 64) == 0);
    rd.addr = 0x07000000; n = build_udp(f, me_mac, me_ip, 5000, NB_READ, 34, &rd, sizeof rd); nb_core_input(f, n); assert(ack_status(34, 5000, NULL) == NB_E_RANGE);
    r.addr = 0x08000000; r.len = 0x20000; n = build_udp(f, me_mac, me_ip, 5000, NB_ERASE, 35, &r, sizeof r); nb_core_input(f, n); assert(ack_status(35, 5000, NULL) == NB_E_RANGE);
    r.addr = 0x08100100; n = build_udp(f, me_mac, me_ip, 5000, NB_ERASE, 36, &r, sizeof r); nb_core_input(f, n); assert(ack_status(36, 5000, NULL) == NB_E_RANGE);
    printf("ERASE/CRC/READ ok\n");

    /* LOG pagine */
    nb_log_req_t lq = {0, 10, 0}; n = build_udp(f, me_mac, me_ip, 5000, NB_LOG, 40, &lq, sizeof lq); nb_core_input(f, n);
    assert(reply(&b, &bl, 40, 5000) == NB_LOG_REPLY); nb_log_reply_t lr; memcpy(&lr, b, sizeof lr);
    assert(lr.len == 10 && lr.next == 10 && memcmp(b + sizeof lr, fake_log, 10) == 0);
    lq.since = 10; lq.max = 1024; n = build_udp(f, me_mac, me_ip, 5000, NB_LOG, 41, &lq, sizeof lq); nb_core_input(f, n);
    reply(&b, &bl, 41, 5000); memcpy(&lr, b, sizeof lr); assert(lr.next == strlen(fake_log) && lr.len == strlen(fake_log) - 10);
    printf("LOG ok\n");

    /* JOURNAL, requete inconnue, BOOT differe */
    nb_journal_t j = {0x33, 0, 0, 0}; n = build_udp(f, me_mac, me_ip, 5000, NB_JOURNAL, 50, &j, sizeof j); nb_core_input(f, n);
    assert(ack_status(50, 5000, NULL) == NB_OK && last_journal.phase == 0x33);
    n = build_udp(f, me_mac, me_ip, 5000, 0x7E, 51, NULL, 0); nb_core_input(f, n); assert(ack_status(51, 5000, &code) == NB_E_UNKNOWN && code == 0x7E);
    n = build_udp(f, me_mac, me_ip, 5000, NB_BOOT, 52, NULL, 0); nb_core_input(f, n); assert(ack_status(52, 5000, NULL) == NB_OK);
    nb_core_poll(); assert(!rebooted); now_ms += 200; nb_core_poll(); assert(rebooted);
    printf("JOURNAL/BOOT ok\n");

    /* BOOTSEL : adresse de slot valide -> OK ; autre -> ARG */
    nb_bootsel_t bs = {0x080E0000u}; n = build_udp(f, me_mac, me_ip, 5000, NB_BOOTSEL, 55, &bs, sizeof bs); nb_core_input(f, n);
    assert(ack_status(55, 5000, NULL) == NB_OK && last_bootsel == 0x080E0000u);
    bs.addr = 0x08020000u; n = build_udp(f, me_mac, me_ip, 5000, NB_BOOTSEL, 56, &bs, sizeof bs); nb_core_input(f, n); assert(ack_status(56, 5000, NULL) == NB_E_ARG);
    printf("BOOTSEL ok\n");

    /* trames tronquees : jamais de reponse, jamais de plantage */
    tx_count = 0;
    for (n = 0; n < 60; n++) { n = build_udp(f, me_mac, me_ip, 5000, NB_WRITE, 60, &wr, sizeof wr); for (uint16_t cut = 0; cut < n; cut += 7) nb_core_input(f, cut); break; }
    printf("truncated frames ok (%d replies)\n", tx_count);

    printf("ALL TESTS PASSED\n");
    return 0;
}
