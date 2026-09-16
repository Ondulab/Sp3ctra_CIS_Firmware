/**
 ******************************************************************************
 * @file           : netboot_core.c
 * @brief          : Coeur portable du flasheur reseau (voir netboot_core.h).
 *
 * Une seule trame d'emission, construite en place et envoyee avant de rendre
 * la main : aucune allocation, aucun etat entre deux requetes hormis la
 * session cliente et le reboot differe.
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance Numerique.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 ******************************************************************************
 */

#include "netboot_core.h"

#include <string.h>

#define ETH_HLEN        14u
#define ETHTYPE_ARP     0x0806u
#define ETHTYPE_IPV4    0x0800u
#define ARP_LEN         28u
#define IP_HLEN         20u
#define IP_PROTO_ICMP   1u
#define IP_PROTO_UDP    17u
#define UDP_HLEN        8u

#define NB_FRAME_MAX    1536u
#define NB_PAYLOAD_OFF  (ETH_HLEN + IP_HLEN + UDP_HLEN)
#define NB_PAYLOAD_MAX  (NB_FRAME_MAX - NB_PAYLOAD_OFF)

#define NB_SESSION_TIMEOUT_MS 10000u
#define NB_REBOOT_DELAY_MS    100u

_Static_assert(sizeof(nb_hdr_t) + sizeof(nb_write_t) + NB_MAX_DATA <= NB_PAYLOAD_MAX,
               "une reponse DATA doit tenir dans une trame");
_Static_assert(sizeof(nb_hdr_t) + sizeof(nb_log_reply_t) + NB_MAX_DATA <= NB_PAYLOAD_MAX,
               "une reponse LOG doit tenir dans une trame");

static nb_netcfg_t me;
static uint8_t     txf[NB_FRAME_MAX] __attribute__((aligned(4)));
static uint16_t    ip_ident;

static struct {
    bool     bound;
    uint8_t  mac[6];
    uint8_t  ip[4];
    uint16_t port;
    uint32_t last;
} cli;

/* Emetteur de la requete en cours de traitement. */
static uint8_t  cur_mac[6];
static uint8_t  cur_ip[4];
static uint16_t cur_port;

static uint32_t last_activity;
static uint32_t requests;
static uint32_t init_tick;
static bool     reboot_pending;
static uint32_t reboot_at;

/* ---- Utilitaires reseau -------------------------------------------------- */

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static void wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFFu);
}

static uint16_t inet_csum(const uint8_t *p, uint32_t len)
{
    uint32_t s = 0;
    while (len > 1u)
    {
        s += rd16(p);
        p += 2;
        len -= 2u;
    }
    if (len != 0u)
    {
        s += (uint32_t)p[0] << 8;
    }
    while ((s >> 16) != 0u)
    {
        s = (s & 0xFFFFu) + (s >> 16);
    }
    return (uint16_t)(~s);
}

static bool ip_is_mine(const uint8_t *ip)
{
    bool all_ones = true;
    bool subnet   = true;

    if (memcmp(ip, me.ip, 4) == 0)
    {
        return true;
    }
    for (int i = 0; i < 4; i++)
    {
        if (ip[i] != 0xFFu)
        {
            all_ones = false;
        }
        if (ip[i] != (uint8_t)(me.ip[i] | (uint8_t)~me.netmask[i]))
        {
            subnet = false;
        }
    }
    return all_ones || subnet;
}

/* ---- Emission ------------------------------------------------------------ */

static uint8_t *eth_begin(const uint8_t *dst, uint16_t type)
{
    memcpy(txf, dst, 6);
    memcpy(txf + 6, me.mac, 6);
    wr16(txf + 12, type);
    return txf + ETH_HLEN;
}

static void ip_begin(uint8_t *ip, uint8_t proto, const uint8_t *dst, uint16_t payload_len)
{
    uint16_t tlen = (uint16_t)(IP_HLEN + payload_len);
    ip[0] = 0x45u;
    ip[1] = 0u;
    wr16(ip + 2, tlen);
    wr16(ip + 4, ip_ident++);
    wr16(ip + 6, 0x4000u); /* DF */
    ip[8] = 64u;
    ip[9] = proto;
    wr16(ip + 10, 0u);
    memcpy(ip + 12, me.ip, 4);
    memcpy(ip + 16, dst, 4);
    wr16(ip + 10, inet_csum(ip, IP_HLEN));
}

/* Charge utile deja placee a txf + NB_PAYLOAD_OFF. Somme UDP a zero : elle est
 * facultative en IPv4 et l'hote ne la verifie pas. */
static void udp_send(const uint8_t *dmac, const uint8_t *dip, uint16_t dport, uint16_t plen)
{
    uint8_t *ip  = eth_begin(dmac, ETHTYPE_IPV4);
    uint8_t *udp = ip + IP_HLEN;
    uint16_t ulen = (uint16_t)(UDP_HLEN + plen);

    wr16(udp, NB_PORT);
    wr16(udp + 2, dport);
    wr16(udp + 4, ulen);
    wr16(udp + 6, 0u);
    ip_begin(ip, IP_PROTO_UDP, dip, ulen);

    nbp_send_frame(txf, (uint16_t)(ETH_HLEN + IP_HLEN + ulen));
}

static uint8_t *snb_payload(void)
{
    return txf + NB_PAYLOAD_OFF + sizeof(nb_hdr_t);
}

static void snb_send(uint8_t type, uint16_t seq, uint16_t len)
{
    nb_hdr_t h;
    h.magic = NB_MAGIC;
    h.type  = type;
    h.flags = 0u;
    h.seq   = seq;
    memcpy(txf + NB_PAYLOAD_OFF, &h, sizeof(h));
    udp_send(cur_mac, cur_ip, cur_port, (uint16_t)(sizeof(h) + len));
}

static void snb_ack(uint16_t seq, uint8_t status, uint32_t code, uint32_t elapsed)
{
    nb_ack_t a;
    memset(&a, 0, sizeof(a));
    a.status     = status;
    a.code       = code;
    a.elapsed_ms = elapsed;
    memcpy(snb_payload(), &a, sizeof(a));
    snb_send(NB_ACK, seq, sizeof(a));
}

static void snb_send_info(uint16_t seq)
{
    nb_info_t    i;
    nb_devinfo_t d;

    memset(&i, 0, sizeof(i));
    memset(&d, 0, sizeof(d));
    nbp_get_info(&d);

    i.proto    = NB_PROTO_VERSION;
    i.state    = cli.bound ? 1u : 0u;
    i.max_data = NB_MAX_DATA;
    memcpy(i.mac, me.mac, 6);
    memcpy(i.ip, me.ip, 4);
    memcpy(i.uid, d.uid, sizeof(i.uid));
    memcpy(i.name, d.name, sizeof(i.name));
    memcpy(i.bl_version, d.bl_version, sizeof(i.bl_version));
    i.journal_phase = d.journal_phase;
    i.trial         = d.trial;
    i.rollback      = d.rollback;
    i.pending       = d.pending;
    i.reset_flags   = d.reset_flags;
    i.uptime_ms     = nbp_tick_ms() - init_tick;
    i.reason        = d.reason;
    i.boot_slot     = d.boot_slot;

    memcpy(snb_payload(), &i, sizeof(i));
    snb_send(NB_INFO, seq, sizeof(i));
}

/* ---- Protocole SNB ------------------------------------------------------- */

static void snb_handle(const uint8_t *smac, const uint8_t *sip, uint16_t sport,
                       const uint8_t *p, uint16_t n)
{
    nb_hdr_t h;
    uint32_t now;

    if (n < sizeof(h))
    {
        return;
    }
    memcpy(&h, p, sizeof(h));
    if (h.magic != NB_MAGIC)
    {
        return;
    }
    p += sizeof(h);
    n  = (uint16_t)(n - sizeof(h));

    memcpy(cur_mac, smac, 6);
    memcpy(cur_ip, sip, 4);
    cur_port = sport;

    now           = nbp_tick_ms();
    last_activity = now;

    if (h.type == NB_DISCOVER)
    {
        snb_send_info(h.seq);
        return;
    }

    /* Session exclusive : un seul hote a la fois, liberee par le silence, par
     * NB_FLAG_RELEASE, ou reprise par le meme hote depuis un autre port (chaque
     * invocation de l'outil ouvre un nouveau socket). */
    if (cli.bound && (now - cli.last) > NB_SESSION_TIMEOUT_MS)
    {
        cli.bound = false;
    }
    if (cli.bound && memcmp(cli.ip, sip, 4) != 0)
    {
        snb_ack(h.seq, NB_E_BUSY, 0u, 0u);
        return;
    }
    if (cli.bound && cli.port != sport)
    {
        cli.bound = false;
    }
    if (!cli.bound)
    {
        cli.bound = true;
        memcpy(cli.mac, smac, 6);
        memcpy(cli.ip, sip, 4);
        cli.port = sport;
    }
    cli.last = now;
    requests++;

    switch (h.type)
    {
    case NB_PING:
        snb_ack(h.seq, NB_OK, 0u, 0u);
        break;

    case NB_ERASE:
    {
        nb_range_t r;
        uint32_t   fail = 0u;
        uint32_t   t0;
        int        st;

        if (n < sizeof(r))
        {
            snb_ack(h.seq, NB_E_ARG, 0u, 0u);
            break;
        }
        memcpy(&r, p, sizeof(r));
        t0 = nbp_tick_ms();
        st = nbp_flash_erase(r.addr, r.len, &fail);
        snb_ack(h.seq, (uint8_t)st, fail, nbp_tick_ms() - t0);
        break;
    }

    case NB_WRITE:
    {
        nb_write_t w;
        uint32_t   fail = 0u;
        int        st;

        if (n < sizeof(w))
        {
            snb_ack(h.seq, NB_E_ARG, 0u, 0u);
            break;
        }
        memcpy(&w, p, sizeof(w));
        if (w.len == 0u || w.len > NB_MAX_DATA || (w.len & 31u) != 0u || (w.addr & 31u) != 0u ||
            n < sizeof(w) + w.len)
        {
            snb_ack(h.seq, NB_E_ARG, w.addr, 0u);
            break;
        }
        st = nbp_flash_write(w.addr, p + sizeof(w), w.len, &fail);
        snb_ack(h.seq, (uint8_t)st, fail, 0u);
        break;
    }

    case NB_READ:
    {
        nb_read_t  r;
        nb_write_t d;
        uint8_t   *out = snb_payload() + sizeof(d);
        int        st;

        if (n < sizeof(r))
        {
            snb_ack(h.seq, NB_E_ARG, 0u, 0u);
            break;
        }
        memcpy(&r, p, sizeof(r));
        if (r.len == 0u || r.len > NB_MAX_DATA)
        {
            snb_ack(h.seq, NB_E_ARG, r.addr, 0u);
            break;
        }
        st = nbp_mem_read(r.addr, out, r.len);
        if (st != NB_OK)
        {
            snb_ack(h.seq, (uint8_t)st, r.addr, 0u);
            break;
        }
        memset(&d, 0, sizeof(d));
        d.addr = r.addr;
        d.len  = r.len;
        memcpy(snb_payload(), &d, sizeof(d));
        snb_send(NB_DATA, h.seq, (uint16_t)(sizeof(d) + r.len));
        break;
    }

    case NB_CRC:
    {
        nb_range_t     r;
        nb_crc_reply_t c;
        uint32_t       crc = 0u;
        int            st;

        if (n < sizeof(r))
        {
            snb_ack(h.seq, NB_E_ARG, 0u, 0u);
            break;
        }
        memcpy(&r, p, sizeof(r));
        st = nbp_crc32(r.addr, r.len, &crc);
        if (st != NB_OK)
        {
            snb_ack(h.seq, (uint8_t)st, r.addr, 0u);
            break;
        }
        c.addr = r.addr;
        c.len  = r.len;
        c.crc  = crc;
        memcpy(snb_payload(), &c, sizeof(c));
        snb_send(NB_CRC_REPLY, h.seq, sizeof(c));
        break;
    }

    case NB_LOG:
    {
        nb_log_req_t   q;
        nb_log_reply_t rep;
        uint8_t       *out  = snb_payload() + sizeof(rep);
        uint32_t       next;
        uint32_t       got;

        if (n < sizeof(q))
        {
            snb_ack(h.seq, NB_E_ARG, 0u, 0u);
            break;
        }
        memcpy(&q, p, sizeof(q));
        if (q.max > NB_MAX_DATA)
        {
            q.max = NB_MAX_DATA;
        }
        next = q.since;
        got  = nbp_log_read(q.src, q.since, out, q.max, &next);
        memset(&rep, 0, sizeof(rep));
        rep.next = next;
        rep.len  = (uint16_t)got;
        memcpy(snb_payload(), &rep, sizeof(rep));
        snb_send(NB_LOG_REPLY, h.seq, (uint16_t)(sizeof(rep) + got));
        break;
    }

    case NB_JOURNAL:
    {
        nb_journal_t j;
        int          st;

        if (n < sizeof(j))
        {
            snb_ack(h.seq, NB_E_ARG, 0u, 0u);
            break;
        }
        memcpy(&j, p, sizeof(j));
        st = nbp_journal_write(j.phase, j.trial, j.rollback, j.pending);
        snb_ack(h.seq, (uint8_t)st, 0u, 0u);
        break;
    }

    case NB_BOOTSEL:
    {
        nb_bootsel_t b;
        int          st;

        if (n < sizeof(b))
        {
            snb_ack(h.seq, NB_E_ARG, 0u, 0u);
            break;
        }
        memcpy(&b, p, sizeof(b));
        st = nbp_boot_select(b.addr);
        snb_ack(h.seq, (uint8_t)st, b.addr, 0u);
        break;
    }

    case NB_BOOT:
        snb_ack(h.seq, NB_OK, 0u, 0u);
        reboot_pending = true;
        reboot_at      = now + NB_REBOOT_DELAY_MS;
        break;

    default:
        snb_ack(h.seq, NB_E_UNKNOWN, h.type, 0u);
        break;
    }

    if ((h.flags & NB_FLAG_RELEASE) != 0u)
    {
        cli.bound = false;
    }
}

/* ---- Reception ----------------------------------------------------------- */

static void handle_arp(const uint8_t *f, uint16_t len)
{
    const uint8_t *a = f + ETH_HLEN;
    uint8_t       *r;

    if (len < ETH_HLEN + ARP_LEN)
    {
        return;
    }
    if (rd16(a) != 1u || rd16(a + 2) != ETHTYPE_IPV4 || a[4] != 6u || a[5] != 4u)
    {
        return;
    }
    if (rd16(a + 6) != 1u) /* requete seulement */
    {
        return;
    }
    if (memcmp(a + 24, me.ip, 4) != 0)
    {
        return;
    }

    r = eth_begin(a + 8, ETHTYPE_ARP);
    wr16(r, 1u);
    wr16(r + 2, ETHTYPE_IPV4);
    r[4] = 6u;
    r[5] = 4u;
    wr16(r + 6, 2u);
    memcpy(r + 8, me.mac, 6);
    memcpy(r + 14, me.ip, 4);
    memcpy(r + 18, a + 8, 10); /* tha/tpa = sha/spa du demandeur */
    nbp_send_frame(txf, ETH_HLEN + ARP_LEN);
}

static void handle_icmp(const uint8_t *f, const uint8_t *ip, const uint8_t *icmp, uint16_t ilen)
{
    uint8_t *rip;
    uint8_t *ri;

    if (ilen < 8u || icmp[0] != 8u || icmp[1] != 0u) /* echo request */
    {
        return;
    }
    if (ilen > NB_FRAME_MAX - ETH_HLEN - IP_HLEN)
    {
        return;
    }

    rip = eth_begin(f + 6, ETHTYPE_IPV4);
    ri  = rip + IP_HLEN;
    memcpy(ri, icmp, ilen);
    ri[0] = 0u; /* echo reply */
    wr16(ri + 2, 0u);
    wr16(ri + 2, inet_csum(ri, ilen));
    ip_begin(rip, IP_PROTO_ICMP, ip + 12, ilen);
    nbp_send_frame(txf, (uint16_t)(ETH_HLEN + IP_HLEN + ilen));
}

static void handle_ip(const uint8_t *f, uint16_t len)
{
    const uint8_t *ip = f + ETH_HLEN;
    const uint8_t *pl;
    uint16_t       ihl;
    uint16_t       tlen;
    uint16_t       plen;

    if (len < ETH_HLEN + IP_HLEN || (ip[0] >> 4) != 4u)
    {
        return;
    }
    ihl  = (uint16_t)((ip[0] & 0x0Fu) * 4u);
    tlen = rd16(ip + 2);
    if (ihl < IP_HLEN || tlen < ihl || (uint32_t)ETH_HLEN + tlen > len)
    {
        return;
    }
    if ((rd16(ip + 6) & 0x3FFFu) != 0u) /* fragments : ignores */
    {
        return;
    }
    if (!ip_is_mine(ip + 16))
    {
        return;
    }

    pl   = ip + ihl;
    plen = (uint16_t)(tlen - ihl);

    if (ip[9] == IP_PROTO_ICMP)
    {
        if (memcmp(ip + 16, me.ip, 4) == 0)
        {
            handle_icmp(f, ip, pl, plen);
        }
    }
    else if (ip[9] == IP_PROTO_UDP)
    {
        uint16_t ulen;
        if (plen < UDP_HLEN || rd16(pl + 2) != NB_PORT)
        {
            return;
        }
        ulen = rd16(pl + 4);
        if (ulen < UDP_HLEN || ulen > plen)
        {
            return;
        }
        snb_handle(f + 6, ip + 12, rd16(pl), pl + UDP_HLEN, (uint16_t)(ulen - UDP_HLEN));
    }
}

void nb_core_input(const uint8_t *frame, uint16_t len)
{
    uint16_t type;

    if (frame == NULL || len < ETH_HLEN)
    {
        return;
    }
    type = rd16(frame + 12);
    if (type == ETHTYPE_ARP)
    {
        handle_arp(frame, len);
    }
    else if (type == ETHTYPE_IPV4)
    {
        handle_ip(frame, len);
    }
}

/* ---- Cycle de vie -------------------------------------------------------- */

void nb_core_init(const nb_netcfg_t *cfg)
{
    memset(&cli, 0, sizeof(cli));
    me             = *cfg;
    ip_ident       = 0u;
    requests       = 0u;
    reboot_pending = false;
    init_tick      = nbp_tick_ms();
    last_activity  = init_tick;
}

void nb_core_poll(void)
{
    uint32_t now = nbp_tick_ms();

    if (cli.bound && (now - cli.last) > NB_SESSION_TIMEOUT_MS)
    {
        cli.bound = false;
    }
    if (reboot_pending && (int32_t)(now - reboot_at) >= 0)
    {
        reboot_pending = false;
        nbp_reboot();
    }
}

bool nb_core_client_bound(void)
{
    return cli.bound;
}

uint32_t nb_core_last_activity(void)
{
    return last_activity;
}

uint32_t nb_core_requests(void)
{
    return requests;
}
