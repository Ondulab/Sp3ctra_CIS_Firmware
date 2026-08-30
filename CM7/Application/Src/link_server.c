/**
 ******************************************************************************
 * @file           : link_server.c
 * @brief          : Sp3ctra Link (SLP) control channel - device side
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
 *
 * One UDP netconn bound on shared_config.network_link_port. Discovery
 * (HELLO/ANNOUNCE), exclusive session (BIND/PING/UNBIND), feedback commands
 * (LED_SET, OLED_OVERLAY) and configuration (CFG_*, CAL_START).
 *
 * Runs in its own task at osPriorityNormal: it may log, it never touches the
 * CIS acquisition path. Everything the CM4 needs lands in shared memory
 * (shared_var.ledState / led_update_requested, shared_feedback).
 */
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "basetypes.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "lwip/api.h"
#include "lwip/ip_addr.h"

#include "globals.h"
#include "config.h"
#include "sp3ctra_link.h"
#include "sys_identity.h"
#include "udp_client.h"
#include "file_manager.h"
#include "icm42688.h"
#include "link_server.h"

/* Private define ------------------------------------------------------------*/
#define LINK_TASK_STACK_BYTES   (4096)
#define LINK_RECV_TIMEOUT_MS    (100)
#define LINK_REBOOT_DELAY_MS    (500)

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
    uint8_t   bound;
    uint32_t  session;
    ip_addr_t peer_ip;
    uint16_t  peer_port;
    ip_addr_t stream_ip;
    uint16_t  stream_port;
    uint32_t  last_ping_tick;
    uint8_t   host_version[3];
} link_session_t;

/* Private variables ---------------------------------------------------------*/
static struct netconn   *link_conn = NULL;
static link_session_t    session;
static volatile uint16_t hid_rate_hz = SLP_DEFAULT_HID_RATE_HZ;
static volatile uint8_t  reboot_pending = 0;
static uint32_t          tx_seq = 0;

static uint8_t rx_buf[SLP_CTRL_MAX_BYTES] __attribute__((aligned(4)));
static uint8_t tx_buf[SLP_CTRL_MAX_BYTES] __attribute__((aligned(4)));

static const osThreadAttr_t linkTask_attributes = {
    .name = "linkTask",
    .stack_size = LINK_TASK_STACK_BYTES,
    .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
static void linkTask(void *argument);
static void link_handle(const uint8_t *buf, uint16_t len, const ip_addr_t *src, uint16_t sport);

/* Helpers -------------------------------------------------------------------*/

static void link_fillHdr(struct slp_hdr *h, uint8_t type, uint16_t len)
{
    h->magic   = SLP_MAGIC;
    h->version = SLP_VERSION;
    h->type    = type;
    h->length  = len;
    h->flags   = 0;
    h->seq     = tx_seq++;
}

static void link_send(const void *msg, uint16_t len, const ip_addr_t *ip, uint16_t port)
{
    if (link_conn == NULL)
    {
        return;
    }

    struct netbuf *buf = netbuf_new();
    if (buf == NULL)
    {
        printf("LINK: netbuf_new failed\n");
        return;
    }

    if (netbuf_ref(buf, msg, len) == ERR_OK)
    {
        err_t err = netconn_sendto(link_conn, buf, ip, port);
        if (err != ERR_OK)
        {
            printf("LINK: sendto failed (%d)\n", (int)err);
        }
    }
    netbuf_delete(buf);
}

static void link_sendError(const ip_addr_t *ip, uint16_t port, uint8_t in_reply_to, uint8_t code)
{
    struct slp_error *e = (struct slp_error *)tx_buf;
    memset(e, 0, sizeof(*e));
    link_fillHdr(&e->hdr, SLP_ERROR, sizeof(*e));
    e->in_reply_to = in_reply_to;
    e->code = code;
    link_send(e, sizeof(*e), ip, port);
}

static void link_publishState(void)
{
    shared_feedback.link_state = session.bound ? 1U : 0U;
    if (session.bound)
    {
        shared_feedback.peer_ip[0] = ip4_addr1(ip_2_ip4(&session.peer_ip));
        shared_feedback.peer_ip[1] = ip4_addr2(ip_2_ip4(&session.peer_ip));
        shared_feedback.peer_ip[2] = ip4_addr3(ip_2_ip4(&session.peer_ip));
        shared_feedback.peer_ip[3] = ip4_addr4(ip_2_ip4(&session.peer_ip));
    }
    else
    {
        memset((void *)shared_feedback.peer_ip, 0, sizeof(shared_feedback.peer_ip));
    }
    __DMB();
    shared_feedback.link_seq++;
}

static void link_applyStreamTarget(void)
{
    if (session.bound)
    {
        udpClient_setTarget(&session.stream_ip, session.stream_port);
    }
    else
    {
        udpClient_applyDefaultTarget();
    }
}

static void link_unbind(const char *reason)
{
    if (session.bound)
    {
        printf("LINK: session 0x%08lX closed (%s)\n", (unsigned long)session.session, reason);
    }
    memset(&session, 0, sizeof(session));
    hid_rate_hz = SLP_DEFAULT_HID_RATE_HZ;
    link_applyStreamTarget();
    link_publishState();
}

static uint16_t link_pixelsPerLine(void)
{
    if (cisConfig.pixels_nb > 0)
    {
        return (uint16_t)cisConfig.pixels_nb;
    }
    return (shared_config.cis_dpi > 200) ? CIS_400DPI_PIXELS_NB : CIS_200DPI_PIXELS_NB;
}

static bool link_sessionMatches(uint32_t sess, const ip_addr_t *src, uint16_t sport)
{
    return session.bound
        && session.session == sess
        && ip_addr_cmp(&session.peer_ip, src)
        && session.peer_port == sport;
}

/* Message handlers ----------------------------------------------------------*/

static void link_handleHello(const ip_addr_t *src, uint16_t sport)
{
    struct slp_announce *a = (struct slp_announce *)tx_buf;
    memset(a, 0, sizeof(*a));
    link_fillHdr(&a->hdr, SLP_ANNOUNCE, sizeof(*a));

    sys_identity_uid(a->uid);
    sys_identity_mac(a->mac);
    a->hw_family     = SLP_HW_CIS;
    a->hw_revision   = HW_REVISION;
    a->fw_version[0] = FW_VERSION_MAJOR;
    a->fw_version[1] = FW_VERSION_MINOR;
    a->fw_version[2] = FW_VERSION_PATCH;
    a->proto_min     = SLP_VERSION;
    sys_identity_name(a->name);
    a->ctrl_port     = shared_config.network_link_port;
    a->stream_port   = shared_config.network_udp_port;
    a->bound         = session.bound;
    if (session.bound)
    {
        memcpy(a->bound_peer_ip, (const void *)shared_feedback.peer_ip, 4);
    }
    a->features      = SLP_FEAT_LED_SET | SLP_FEAT_OLED_OVERLAY | SLP_FEAT_CFG | SLP_FEAT_CAL
                     | SLP_FEAT_HID_BUTTONS | SLP_FEAT_HID_ACC | SLP_FEAT_HID_GYRO | SLP_FEAT_HID_TEMP;
    a->n_buttons     = NUMBER_OF_BUTTONS;
    a->n_leds        = NUMBER_OF_LEDS;
    a->led_kind      = SLP_LED_MONO_PWM;
    a->imu_kind      = SLP_IMU_6AXIS;
    a->display_w     = 256;
    a->display_h     = 64;
    a->display_bpp   = 4;
    a->n_dpi         = 2;
    a->dpi[0]        = 200;
    a->dpi[1]        = 400;
    a->pixels_at_dpi[0] = CIS_200DPI_PIXELS_NB;
    a->pixels_at_dpi[1] = CIS_400DPI_PIXELS_NB;
    a->line_rate_max = 1000;
    a->hid_rate_max  = SLP_MAX_HID_RATE_HZ;

    link_send(a, sizeof(*a), src, sport);
}

static void link_handleBind(const struct slp_bind *m, const ip_addr_t *src, uint16_t sport)
{
    struct slp_bind_ack *ack = (struct slp_bind_ack *)tx_buf;
    memset(ack, 0, sizeof(*ack));
    link_fillHdr(&ack->hdr, SLP_BIND_ACK, sizeof(*ack));
    ack->session = m->session;

    const bool same_peer = session.bound
                        && ip_addr_cmp(&session.peer_ip, src)
                        && session.peer_port == sport;

    if (session.bound && !same_peer)
    {
        ack->status = SLP_BIND_BUSY;
        link_send(ack, sizeof(*ack), src, sport);
        return;
    }

    if (m->stream_mode > SLP_STREAM_MULTICAST)
    {
        ack->status = SLP_BIND_BAD_PARAM;
        link_send(ack, sizeof(*ack), src, sport);
        return;
    }

    /* Accept (a re-BIND from the same peer simply refreshes the session). */
    const bool was_bound = session.bound;
    session.bound     = 1;
    session.session   = m->session;
    ip_addr_copy(session.peer_ip, *src);
    session.peer_port = sport;
    session.stream_port = (m->stream_port != 0) ? m->stream_port : SLP_STREAM_PORT;
    if (m->stream_mode == SLP_STREAM_MULTICAST)
    {
        IP4_ADDR(ip_2_ip4(&session.stream_ip), m->mcast_group[0], m->mcast_group[1], m->mcast_group[2], m->mcast_group[3]);
    }
    else
    {
        ip_addr_copy(session.stream_ip, *src);
    }
    uint16_t rate = (m->hid_rate_hz != 0) ? m->hid_rate_hz : SLP_DEFAULT_HID_RATE_HZ;
    if (rate > SLP_MAX_HID_RATE_HZ)
    {
        rate = SLP_MAX_HID_RATE_HZ;
    }
    hid_rate_hz = rate;
    memcpy(session.host_version, m->host_version, 3);
    session.last_ping_tick = HAL_GetTick();

    link_applyStreamTarget();
    link_publishState();

    const uint16_t pixels = link_pixelsPerLine();
    ack->status           = SLP_BIND_OK;
    ack->dpi              = shared_config.cis_dpi;
    ack->pixels_per_line  = pixels;
    ack->fragment_pixels  = UDP_LINE_FRAGMENT_SIZE;
    ack->fragment_count   = (uint8_t)((pixels + UDP_LINE_FRAGMENT_SIZE - 1) / UDP_LINE_FRAGMENT_SIZE);
    ack->line_packet_bytes = (uint16_t)SLP_LINE_BYTES(UDP_LINE_FRAGMENT_SIZE);
    ack->hid_rate_hz      = hid_rate_hz;
    ack->hid_valid_mask   = SLP_HID_BUTTONS | SLP_HID_ACC | SLP_HID_GYRO | SLP_HID_TEMP;
    ack->session_timeout_ms = SLP_SESSION_TIMEOUT_MS;

    link_send(ack, sizeof(*ack), src, sport);

    if (!was_bound)
    {
        printf("LINK: session 0x%08lX bound to %s:%u (host %u.%u.%u), stream -> port %u, HID %u Hz\n",
               (unsigned long)session.session, ipaddr_ntoa(src), (unsigned)sport,
               (unsigned)m->host_version[0], (unsigned)m->host_version[1], (unsigned)m->host_version[2],
               (unsigned)session.stream_port, (unsigned)hid_rate_hz);
    }
}

static void link_handlePing(const struct slp_ping *m, const ip_addr_t *src, uint16_t sport)
{
    if (!link_sessionMatches(m->session, src, sport))
    {
        link_sendError(src, sport, SLP_PING, SLP_ERR_NOT_BOUND);
        return;
    }
    session.last_ping_tick = HAL_GetTick();

    struct slp_pong *p = (struct slp_pong *)tx_buf;
    memset(p, 0, sizeof(*p));
    link_fillHdr(&p->hdr, SLP_PONG, sizeof(*p));
    p->session       = m->session;
    p->host_time_ms  = m->host_time_ms;
    p->uptime_ms     = HAL_GetTick();
    p->lines_sent    = udpClient_linesSent();
    p->line_rate_lps = (uint16_t)((shared_var.cis_freq < 0) ? 0 : (shared_var.cis_freq > 65535 ? 65535 : shared_var.cis_freq));
    p->cal_state     = (uint8_t)shared_var.cis_cal_state;
    p->cal_progress  = (uint8_t)((shared_var.cis_cal_progressbar > 100U) ? 100U : shared_var.cis_cal_progressbar);
    p->temp_c_x10    = (int16_t)(icm42688_temp() * 10.0f);
    p->link_flags    = udpClient_isStreaming() ? SLP_LINK_STREAMING : 0;
    if (shared_var.cis_cal_state != CIS_CAL_END && shared_var.cis_cal_state != CIS_CAL_REQUESTED)
    {
        p->link_flags |= SLP_LINK_CAL_RUNNING;
    }
    link_send(p, sizeof(*p), src, sport);
}

static void link_handleLedSet(const struct slp_led_set *m)
{
    uint8_t no_local = shared_feedback.led_no_local_press;

    for (uint32_t i = 0; i < NUMBER_OF_LEDS && i < SLP_MAX_LEDS; i++)
    {
        if ((m->led_mask & (1U << i)) == 0)
        {
            continue;
        }
        const struct slp_led_cmd *c = &m->led[i];
        volatile struct led_State *s = &shared_var.ledState[i];
        s->brightness_1 = (c->brightness_1 > 100U) ? 100U : c->brightness_1;
        s->time_1       = c->time_1_ms;
        s->glide_1      = (c->glide_1 > 100U) ? 100U : c->glide_1;
        s->brightness_2 = (c->brightness_2 > 100U) ? 100U : c->brightness_2;
        s->time_2       = c->time_2_ms;
        s->glide_2      = (c->glide_2 > 100U) ? 100U : c->glide_2;
        s->blink_count  = c->blink_count;
        __DMB();
        shared_var.led_update_requested[i] = TRUE;

        if (c->flags & SLP_LED_NO_LOCAL_PRESS)
        {
            no_local |= (uint8_t)(1U << i);
        }
        else
        {
            no_local &= (uint8_t)~(1U << i);
        }
    }
    shared_feedback.led_no_local_press = no_local;
}

static void link_handleOverlay(const struct slp_oled_overlay *m)
{
    memcpy((void *)&shared_feedback.overlay, m, sizeof(struct slp_oled_overlay));
    if (shared_feedback.overlay.count > SLP_OVERLAY_MAX_ITEMS)
    {
        shared_feedback.overlay.count = SLP_OVERLAY_MAX_ITEMS;
    }
    __DMB();
    shared_feedback.overlay_seq++;
}

static void link_handleOverlayClear(void)
{
    shared_feedback.overlay.count = 0;
    __DMB();
    shared_feedback.overlay_seq++;
}

/* CFG ------------------------------------------------------------------------*/

static uint32_t cfg_packIp(const volatile uint8_t ip[4])
{
    return (uint32_t)ip[0] | ((uint32_t)ip[1] << 8) | ((uint32_t)ip[2] << 16) | ((uint32_t)ip[3] << 24);
}

static void cfg_unpackIp(uint32_t v, volatile uint8_t ip[4])
{
    ip[0] = (uint8_t)(v);
    ip[1] = (uint8_t)(v >> 8);
    ip[2] = (uint8_t)(v >> 16);
    ip[3] = (uint8_t)(v >> 24);
}

static uint32_t cfg_f32bits(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

static float cfg_bitsf32(uint32_t u)
{
    float f;
    memcpy(&f, &u, 4);
    return f;
}

/** Fill item->type/value from the live configuration. Returns 0 for unknown ids. */
static int cfg_read(struct slp_cfg_item *it)
{
    it->flags = 0;
    switch (it->id)
    {
        case SLP_CFG_DPI:             it->type = SLP_CFG_U16; it->value = shared_config.cis_dpi; break;
        case SLP_CFG_OVERSAMPLING:    it->type = SLP_CFG_U8;  it->value = shared_config.cis_oversampling; break;
        case SLP_CFG_HANDEDNESS:      it->type = SLP_CFG_U8;  it->value = shared_config.cis_handedness; break;
        case SLP_CFG_GYRO_FS:         it->type = SLP_CFG_U8;  it->value = shared_config.imu_gyro_sensitivity; break;
        case SLP_CFG_ACCEL_FS:        it->type = SLP_CFG_U8;  it->value = shared_config.imu_accel_sensitivity; break;
        case SLP_CFG_GUI_SHOW_IMU:    it->type = SLP_CFG_U8;  it->value = shared_config.gui_show_imu; break;
        case SLP_CFG_GUI_INVERT:      it->type = SLP_CFG_U8;  it->value = shared_config.gui_invert_cis_image; break;
        case SLP_CFG_SCREENSAVER_S:   it->type = SLP_CFG_U16; it->value = shared_config.screensaver_timeout_sec; break;
        case SLP_CFG_MOTION_THR_ACC:  it->type = SLP_CFG_F32; it->value = cfg_f32bits(shared_config.motion_threshold_acc); break;
        case SLP_CFG_MOTION_THR_GYRO: it->type = SLP_CFG_F32; it->value = cfg_f32bits(shared_config.motion_threshold_gyro); break;
        case SLP_CFG_NET_IP:          it->type = SLP_CFG_IP4; it->value = cfg_packIp(shared_config.network_ip); break;
        case SLP_CFG_NET_MASK:        it->type = SLP_CFG_IP4; it->value = cfg_packIp(shared_config.network_netmask); break;
        case SLP_CFG_NET_GW:          it->type = SLP_CFG_IP4; it->value = cfg_packIp(shared_config.network_gw); break;
        case SLP_CFG_NET_DEST_IP:     it->type = SLP_CFG_IP4; it->value = cfg_packIp(shared_config.network_dest_ip); break;
        case SLP_CFG_STREAM_PORT:     it->type = SLP_CFG_U16; it->value = shared_config.network_udp_port; break;
        case SLP_CFG_STREAM_WHEN_UNBOUND: it->type = SLP_CFG_U8; it->value = shared_config.stream_when_unbound; break;
        case SLP_CFG_LINK_PORT:       it->type = SLP_CFG_U16; it->value = shared_config.network_link_port; break;
        case SLP_CFG_LINE_RATE:       it->type = SLP_CFG_U16; it->value = (uint32_t)((shared_var.cis_freq < 0) ? 0 : shared_var.cis_freq); it->flags = SLP_CFG_F_READONLY; break;
        default:
            it->type = SLP_CFG_U32;
            it->value = 0;
            it->flags = SLP_CFG_F_UNKNOWN;
            return 0;
    }
    return 1;
}

/** Apply one item. Sets *changed / *reboot; the item is rewritten with the stored value + flags. */
static void cfg_write(struct slp_cfg_item *it, bool *changed, bool *reboot, bool *stream_target)
{
    const uint32_t v = it->value;
    uint8_t flags = 0;

    switch (it->id)
    {
        case SLP_CFG_DPI:
            if (v != 200U && v != 400U) { flags = SLP_CFG_F_REJECTED; break; }
            if (shared_config.cis_dpi != v) { shared_config.cis_dpi = (uint16_t)v; *changed = true; *reboot = true; }
            flags = SLP_CFG_F_REBOOT;
            break;
        case SLP_CFG_OVERSAMPLING:
            if (v < 1U || v > 32U) { flags = SLP_CFG_F_REJECTED; break; }
            shared_config.cis_oversampling = (uint8_t)v; *changed = true;
            break;
        case SLP_CFG_HANDEDNESS:
            if (v > 1U) { flags = SLP_CFG_F_REJECTED; break; }
            shared_config.cis_handedness = (uint8_t)v; *changed = true;
            break;
        case SLP_CFG_GYRO_FS:
            if (v > 0x07U) { flags = SLP_CFG_F_REJECTED; break; }
            shared_config.imu_gyro_sensitivity = (uint8_t)v; *changed = true;
            icm42688_setGyroFS((GyroFS)v);
            if (icm42688_calibrateGyro() == ICM42688_OK)
            {
                icm42688_saveCalibration(IMU_CALIBRATION_FILE_PATH);
            }
            break;
        case SLP_CFG_ACCEL_FS:
            if (v > 0x03U) { flags = SLP_CFG_F_REJECTED; break; }
            shared_config.imu_accel_sensitivity = (uint8_t)v; *changed = true;
            icm42688_setAccelFS((AccelFS)v);
            if (icm42688_performCalibration() != ICM42688_OK)
            {
                printf("LINK: IMU recalibration failed after accel FS change\n");
            }
            break;
        case SLP_CFG_GUI_SHOW_IMU:
            shared_config.gui_show_imu = (v > 0U) ? 1U : 0U; *changed = true;
            break;
        case SLP_CFG_GUI_INVERT:
            shared_config.gui_invert_cis_image = (v > 0U) ? 1U : 0U; *changed = true;
            break;
        case SLP_CFG_SCREENSAVER_S:
            if (v < MIN_SCREENSAVER_TIMEOUT_SEC || v > MAX_SCREENSAVER_TIMEOUT_SEC) { flags = SLP_CFG_F_REJECTED; break; }
            shared_config.screensaver_timeout_sec = (uint16_t)v; *changed = true;
            break;
        case SLP_CFG_MOTION_THR_ACC:
        {
            const float f = cfg_bitsf32(v);
            if (!(f >= MIN_MOTION_THRESHOLD_ACC && f <= MAX_MOTION_THRESHOLD_ACC)) { flags = SLP_CFG_F_REJECTED; break; }
            shared_config.motion_threshold_acc = f; *changed = true;
            break;
        }
        case SLP_CFG_MOTION_THR_GYRO:
        {
            const float f = cfg_bitsf32(v);
            if (!(f >= MIN_MOTION_THRESHOLD_GYRO && f <= MAX_MOTION_THRESHOLD_GYRO)) { flags = SLP_CFG_F_REJECTED; break; }
            shared_config.motion_threshold_gyro = f; *changed = true;
            break;
        }
        case SLP_CFG_NET_IP:
            cfg_unpackIp(v, shared_config.network_ip); *changed = true; *reboot = true; flags = SLP_CFG_F_REBOOT;
            break;
        case SLP_CFG_NET_MASK:
            cfg_unpackIp(v, shared_config.network_netmask); *changed = true; *reboot = true; flags = SLP_CFG_F_REBOOT;
            break;
        case SLP_CFG_NET_GW:
            cfg_unpackIp(v, shared_config.network_gw); *changed = true; *reboot = true; flags = SLP_CFG_F_REBOOT;
            break;
        case SLP_CFG_NET_DEST_IP:
            cfg_unpackIp(v, shared_config.network_dest_ip); *changed = true; *stream_target = true;
            break;
        case SLP_CFG_STREAM_PORT:
            if (v < 1U || v > 65535U) { flags = SLP_CFG_F_REJECTED; break; }
            shared_config.network_udp_port = (uint16_t)v; *changed = true; *stream_target = true;
            break;
        case SLP_CFG_STREAM_WHEN_UNBOUND:
            shared_config.stream_when_unbound = (v > 0U) ? 1U : 0U; *changed = true; *stream_target = true;
            break;
        case SLP_CFG_LINK_PORT:
            if (v < 1U || v > 65535U) { flags = SLP_CFG_F_REJECTED; break; }
            shared_config.network_link_port = (uint16_t)v; *changed = true; *reboot = true; flags = SLP_CFG_F_REBOOT;
            break;
        case SLP_CFG_LINE_RATE:
            flags = SLP_CFG_F_READONLY | SLP_CFG_F_REJECTED;
            break;
        default:
            flags = SLP_CFG_F_UNKNOWN;
            break;
    }

    /* Echo the stored value back (or the untouched one when rejected). */
    (void)cfg_read(it);
    it->flags |= flags;
}

static void link_handleCfg(const struct slp_cfg_msg *m, bool set, const ip_addr_t *src, uint16_t sport)
{
    struct slp_cfg_msg *r = (struct slp_cfg_msg *)tx_buf;
    memset(r, 0, sizeof(*r));
    link_fillHdr(&r->hdr, SLP_CFG_REPLY, sizeof(*r));

    const uint8_t count = (m->count > SLP_CFG_MAX_ITEMS) ? SLP_CFG_MAX_ITEMS : m->count;
    r->count = count;

    bool changed = false, reboot = false, stream_target = false;

    for (uint8_t i = 0; i < count; i++)
    {
        r->item[i] = m->item[i];
        if (set)
        {
            cfg_write(&r->item[i], &changed, &reboot, &stream_target);
        }
        else
        {
            (void)cfg_read(&r->item[i]);
        }
    }

    if (changed)
    {
        file_writeConfig(CONFIG_FILE_PATH, &shared_config);
    }
    if (stream_target && !session.bound)
    {
        udpClient_applyDefaultTarget();
    }

    link_send(r, sizeof(*r), src, sport);

    if (reboot)
    {
        printf("LINK: configuration change requires a reboot\n");
        reboot_pending = 1;
    }
}

static void link_handleCal(const struct slp_cal_start *m, const ip_addr_t *src, uint16_t sport)
{
    if (m->kind == SLP_CAL_CIS)
    {
        printf("LINK: CIS calibration requested\n");
        shared_var.cis_cal_state = CIS_CAL_REQUESTED;
    }
    else if (m->kind == SLP_CAL_IMU)
    {
        printf("LINK: IMU calibration requested\n");
        /* Blocking (~1.2 s): keep the device still. PING/PONG resumes afterwards,
         * well within the session timeout. */
        if (icm42688_performCalibration() != ICM42688_OK)
        {
            printf("LINK: IMU calibration failed\n");
        }
    }
    else
    {
        link_sendError(src, sport, SLP_CAL_START, SLP_ERR_BAD_PARAM);
    }
}

/* Dispatcher ----------------------------------------------------------------*/

static void link_handle(const uint8_t *buf, uint16_t len, const ip_addr_t *src, uint16_t sport)
{
    const struct slp_hdr *h = (const struct slp_hdr *)buf;

    if (len < sizeof(struct slp_hdr) || h->magic != SLP_MAGIC)
    {
        return;   /* not ours: silently ignored */
    }
    if (h->version != SLP_VERSION)
    {
        link_sendError(src, sport, h->type, SLP_ERR_BAD_VERSION);
        return;
    }
    if (h->length != len)
    {
        link_sendError(src, sport, h->type, SLP_ERR_BAD_LENGTH);
        return;
    }

#define LINK_NEED(structtype)                                                   \
    if (len < sizeof(struct structtype)) { link_sendError(src, sport, h->type, SLP_ERR_BAD_LENGTH); return; }

    switch (h->type)
    {
        case SLP_HELLO:
            LINK_NEED(slp_hello);
            link_handleHello(src, sport);
            break;

        case SLP_BIND:
            LINK_NEED(slp_bind);
            link_handleBind((const struct slp_bind *)buf, src, sport);
            break;

        case SLP_UNBIND:
        {
            LINK_NEED(slp_unbind);
            const struct slp_unbind *m = (const struct slp_unbind *)buf;
            if (link_sessionMatches(m->session, src, sport))
            {
                link_unbind("unbind");
            }
            break;
        }

        case SLP_PING:
            LINK_NEED(slp_ping);
            link_handlePing((const struct slp_ping *)buf, src, sport);
            break;

        case SLP_LED_SET:
            LINK_NEED(slp_led_set);
            if (!session.bound) { link_sendError(src, sport, h->type, SLP_ERR_NOT_BOUND); return; }
            link_handleLedSet((const struct slp_led_set *)buf);
            break;

        case SLP_OLED_OVERLAY:
            LINK_NEED(slp_oled_overlay);
            if (!session.bound) { link_sendError(src, sport, h->type, SLP_ERR_NOT_BOUND); return; }
            link_handleOverlay((const struct slp_oled_overlay *)buf);
            break;

        case SLP_OLED_CLEAR:
            if (!session.bound) { link_sendError(src, sport, h->type, SLP_ERR_NOT_BOUND); return; }
            link_handleOverlayClear();
            break;

        case SLP_CFG_GET:
        case SLP_CFG_SET:
            LINK_NEED(slp_cfg_msg);
            if (!session.bound) { link_sendError(src, sport, h->type, SLP_ERR_NOT_BOUND); return; }
            link_handleCfg((const struct slp_cfg_msg *)buf, h->type == SLP_CFG_SET, src, sport);
            break;

        case SLP_CAL_START:
            LINK_NEED(slp_cal_start);
            if (!session.bound) { link_sendError(src, sport, h->type, SLP_ERR_NOT_BOUND); return; }
            link_handleCal((const struct slp_cal_start *)buf, src, sport);
            break;

        default:
            link_sendError(src, sport, h->type, SLP_ERR_UNKNOWN_TYPE);
            break;
    }
#undef LINK_NEED
}

/* Task ----------------------------------------------------------------------*/

static void linkTask(void *argument)
{
    (void)argument;

    link_conn = netconn_new(NETCONN_UDP);
    if (link_conn == NULL)
    {
        printf("LINK: netconn_new failed\n");
        vTaskDelete(NULL);
        return;
    }

    if (netconn_bind(link_conn, IP_ADDR_ANY, shared_config.network_link_port) != ERR_OK)
    {
        printf("LINK: bind on port %u failed\n", (unsigned)shared_config.network_link_port);
        netconn_delete(link_conn);
        link_conn = NULL;
        vTaskDelete(NULL);
        return;
    }
    netconn_set_recvtimeout(link_conn, LINK_RECV_TIMEOUT_MS);

    {
        char name[SYS_IDENTITY_NAME_LEN];
        sys_identity_name(name);
        printf("LINK: %s listening on UDP %u (SLP v%u)\n", name, (unsigned)shared_config.network_link_port, (unsigned)SLP_VERSION);
    }

    link_unbind("startup");

    for (;;)
    {
        struct netbuf *nb = NULL;
        const err_t err = netconn_recv(link_conn, &nb);

        if (err == ERR_OK && nb != NULL)
        {
            const uint16_t len = netbuf_len(nb);
            if (len >= sizeof(struct slp_hdr) && len <= sizeof(rx_buf))
            {
                netbuf_copy(nb, rx_buf, len);
                /* Copy the source before releasing the netbuf. */
                ip_addr_t src;
                ip_addr_copy(src, *netbuf_fromaddr(nb));
                const uint16_t sport = netbuf_fromport(nb);
                netbuf_delete(nb);
                nb = NULL;
                link_handle(rx_buf, len, &src, sport);
            }
            if (nb != NULL)
            {
                netbuf_delete(nb);
            }
        }
        else if (nb != NULL)
        {
            netbuf_delete(nb);
        }

        /* Session keep-alive */
        if (session.bound && (HAL_GetTick() - session.last_ping_tick) > SLP_SESSION_TIMEOUT_MS)
        {
            link_unbind("timeout");
        }

        if (reboot_pending)
        {
            osDelay(LINK_REBOOT_DELAY_MS);
            System_SafeReset();
        }
    }
}

/* Public API ----------------------------------------------------------------*/

LINKSERVER_StatusTypeDef link_serverInit(void)
{
    memset(&session, 0, sizeof(session));

    if (osThreadNew(linkTask, NULL, &linkTask_attributes) == NULL)
    {
        printf("LINK: failed to create link task\n");
        return LINKSERVER_ERROR;
    }
    return LINKSERVER_OK;
}

uint8_t link_isBound(void)
{
    return session.bound;
}

uint16_t link_getHidRateHz(void)
{
    return hid_rate_hz;
}

void link_getPeerIp(uint8_t out[4])
{
    memcpy(out, (const void *)shared_feedback.peer_ip, 4);
}

void link_refreshStreamTarget(void)
{
    if (!session.bound)
    {
        udpClient_applyDefaultTarget();
    }
}
