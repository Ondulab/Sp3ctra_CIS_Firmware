/*
 * Simulateur hote du flasheur reseau : le coeur reel (netboot_core.c) avec
 * une flash en RAM, expose sur un tunnel UDP local. Les datagrammes SNB recus
 * sont enveloppes dans une trame Ethernet/IPv4/UDP factice, la reponse du
 * coeur est desenveloppee et renvoyee au client. Permet de tester
 * netflash.py de bout en bout sans materiel.
 *
 *   scripts/netboot/sim/build.sh
 *   scripts/netboot/sim/netboot_sim 55152 &
 *   scripts/netboot/netflash.py --host 127.0.0.1 flash --cm7 fw.bin --no-enter --wait-app 0
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <zlib.h>

#include "netboot_core.h"

#define FLASH_BASE 0x08000000u
#define FLASH_SIZE 0x00200000u
static uint8_t flash[FLASH_SIZE];
static int sock;
static struct sockaddr_in client;
static char logbuf[65536];
static uint32_t loghead;
static int erase_delay_ms = 300;

static void lg(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void lg(const char *fmt, ...) {
    char line[256]; va_list ap; va_start(ap, fmt); int n = vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    if (n < 0) return; if ((size_t)n > sizeof line - 1) n = sizeof line - 1;
    fputs(line, stdout); fflush(stdout);
    for (int i = 0; i < n; i++) logbuf[loghead++ % sizeof logbuf] = (uint8_t)line[i];
}

static uint32_t t0_ms;
uint32_t nbp_tick_ms(void) { struct timeval tv; gettimeofday(&tv, NULL); return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000) - t0_ms; }
void nbp_get_info(nb_devinfo_t *info) { memset(info, 0, sizeof *info); memcpy(info->uid, "SIMULATOR-01", 12); strcpy(info->name, "Sp3ctra-SIM0"); strcpy(info->bl_version, "sim"); info->journal_phase = 0x11; info->reset_flags = 1u << 24; info->reason = NB_REASON_MAILBOX; }
static int in_flash(uint32_t a, uint32_t l) { return l && a >= NB_FLASH_START + 0x20000u && a < NB_FLASH_END && l <= NB_FLASH_END - a; }
int nbp_flash_erase(uint32_t addr, uint32_t len, uint32_t *fail) {
    if (!in_flash(addr, len) || (addr & 0x1FFFF)) { *fail = addr; return NB_E_RANGE; }
    for (uint32_t a = addr; a < addr + len; a += 0x20000) { lg("[B %8.3f] NETBOOT: erase 0x%08X\n", nbp_tick_ms() / 1000.0, a); memset(flash + (a - FLASH_BASE), 0xFF, 0x20000); usleep(erase_delay_ms * 1000); }
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
    usleep(len / 32 * 130); /* 130 us par mot, comme la flash H7 en x32 */
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
    (void)src; uint32_t oldest = loghead > sizeof logbuf ? loghead - sizeof logbuf : 0;
    if (since < oldest) since = oldest; if (since > loghead) since = loghead;
    uint32_t n = loghead - since; if (n > max) n = max;
    for (uint32_t i = 0; i < n; i++) out[i] = (uint8_t)logbuf[(since + i) % sizeof logbuf];
    *next = since + n; return n;
}
int nbp_journal_write(uint8_t phase, uint8_t trial, uint8_t rollback, uint8_t pending) {
    lg("[B %8.3f] NETBOOT: journal phase 0x%02X (%u/%u/%u)\n", nbp_tick_ms() / 1000.0, phase, trial, rollback, pending);
    return (phase == 0 || phase == 0x11 || phase == 0x22 || phase == 0x33 || phase == 0x44 || phase == 0x55) ? NB_OK : NB_E_ARG;
}
int nbp_boot_select(uint32_t addr) { lg("[B %8.3f] NETBOOT: BOOT_ADD0 -> 0x%08X\n", nbp_tick_ms() / 1000.0, addr); return (addr == 0x08000000u || addr == 0x080E0000u) ? NB_OK : NB_E_ARG; }
void nbp_reboot(void) { lg("[B %8.3f] NETBOOT: rebooting (simulator exits)\n", nbp_tick_ms() / 1000.0); exit(0); }

/* La reponse du coeur est une trame complete : on en extrait la charge UDP. */
void nbp_send_frame(const uint8_t *frame, uint16_t len) {
    if (len < 42 || frame[12] != 0x08 || frame[13] != 0x00) return;
    const uint8_t *ip = frame + 14; uint16_t ihl = (ip[0] & 0x0F) * 4; if (ip[9] != 17) return;
    const uint8_t *udp = ip + ihl; uint16_t ulen = (udp[4] << 8) | udp[5];
    sendto(sock, udp + 8, ulen - 8, 0, (struct sockaddr *)&client, sizeof client);
}

static const uint8_t me_mac[6] = {0x02, 0x53, 0x33, 0x41, 0x00, 0x01};
static const uint8_t host_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

static uint16_t csum(const uint8_t *p, uint32_t len) { uint32_t s = 0; while (len > 1) { s += (p[0] << 8) | p[1]; p += 2; len -= 2; } if (len) s += p[0] << 8; while (s >> 16) s = (s & 0xFFFF) + (s >> 16); return (uint16_t)~s; }

int main(int argc, char **argv) {
    int port = argc > 1 ? atoi(argv[1]) : NB_PORT;
    t0_ms = nbp_tick_ms();
    if (argc > 2) erase_delay_ms = atoi(argv[2]);
    nb_netcfg_t cfg; memcpy(cfg.mac, me_mac, 6);
    uint8_t ip[4] = {127, 0, 0, 1}; memcpy(cfg.ip, ip, 4); memset(cfg.netmask, 255, 3); cfg.netmask[3] = 0;
    memset(flash, 0xFF, sizeof flash);
    /* zone bootloader "programmee" pour que l'interdiction soit visible */
    memset(flash, 0xA5, 0x20000);
    nb_core_init(&cfg);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in me = {0}; me.sin_family = AF_INET; me.sin_port = htons(port); me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&me, sizeof me) < 0) { perror("bind"); return 1; }
    lg("[B    0.000] NETBOOT: simulator listening on UDP %d (erase delay %d ms/sector)\n", port, erase_delay_ms);

    for (;;) {
        uint8_t payload[2048], frame[2048];
        struct timeval tv = {0, 50000};
        fd_set fds; FD_ZERO(&fds); FD_SET(sock, &fds);
        int r = select(sock + 1, &fds, NULL, NULL, &tv);
        if (r > 0) {
            socklen_t cl = sizeof client;
            ssize_t n = recvfrom(sock, payload, sizeof payload, 0, (struct sockaddr *)&client, &cl);
            if (n <= 0) continue;
            /* enveloppe Ethernet + IPv4 + UDP vers notre IP, depuis 127.0.0.1:port_client */
            memcpy(frame, me_mac, 6); memcpy(frame + 6, host_mac, 6); frame[12] = 0x08; frame[13] = 0x00;
            uint8_t *iph = frame + 14, *udph = iph + 20;
            uint16_t ulen = (uint16_t)(8 + n), tlen = (uint16_t)(20 + ulen);
            iph[0] = 0x45; iph[1] = 0; iph[2] = tlen >> 8; iph[3] = tlen & 0xFF; iph[4] = 0; iph[5] = 1; iph[6] = 0; iph[7] = 0; iph[8] = 64; iph[9] = 17; iph[10] = iph[11] = 0;
            memcpy(iph + 12, ip, 4); memcpy(iph + 16, ip, 4); uint16_t c = csum(iph, 20); iph[10] = c >> 8; iph[11] = c & 0xFF;
            uint16_t sport = ntohs(client.sin_port);
            udph[0] = sport >> 8; udph[1] = sport & 0xFF; udph[2] = NB_PORT >> 8; udph[3] = NB_PORT & 0xFF; udph[4] = ulen >> 8; udph[5] = ulen & 0xFF; udph[6] = udph[7] = 0;
            memcpy(udph + 8, payload, (size_t)n);
            nb_core_input(frame, (uint16_t)(14 + tlen));
        }
        nb_core_poll();
    }
}
