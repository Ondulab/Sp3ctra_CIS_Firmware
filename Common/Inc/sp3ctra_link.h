/**
 ******************************************************************************
 * @file           : sp3ctra_link.h
 * @brief          : Sp3ctra Link Protocol (SLP) v1 - wire contract
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
 * SINGLE SOURCE OF TRUTH for the device <-> host protocol.
 *
 * This header is copied byte-for-byte into the Sp3ctra VST repository
 * (vst/source/communication/link/sp3ctra_link.h). It must stay plain C99
 * (+ <stdint.h>), carry no code and depend on nothing else.
 *
 * Wire rules
 * ----------
 *  - UDP. Two flows: CONTROL (bidirectional, device listens on SLP_CTRL_PORT,
 *    replies go to the sender address:port) and STREAM (device -> host, port
 *    chosen by the host in BIND, default SLP_STREAM_PORT).
 *  - Little-endian, packed structures, fixed-width fields only.
 *  - Every datagram starts with struct slp_hdr. hdr.length is the total
 *    datagram length, used to reject truncated packets.
 *  - A receiver ignores unknown message types and any bytes beyond the
 *    structure it knows (reserved fields exist for that purpose). Breaking
 *    changes bump SLP_VERSION; slp_announce.proto_min tells the host which
 *    versions the device still accepts.
 *  - LINE planes (r, g, b) are contiguous, each pixel_count bytes long,
 *    immediately after struct slp_line_hdr. The number of pixels per fragment
 *    is NEGOTIATED (slp_bind_ack.fragment_pixels), never assumed.
 *
 * Session
 * -------
 *   host: HELLO (broadcast or unicast)   -> device: ANNOUNCE (unicast)
 *   host: BIND {session, stream target}  -> device: BIND_ACK {status, layout}
 *   host: PING every SLP_PING_PERIOD_MS  -> device: PONG {status}
 *   host: UNBIND                          (best effort)
 *   The device drops the session after SLP_SESSION_TIMEOUT_MS without PING.
 *   One session at a time: a BIND from another peer is answered BUSY.
 */
#ifndef SP3CTRA_LINK_H
#define SP3CTRA_LINK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants                                                                  */
/* -------------------------------------------------------------------------- */
#define SLP_MAGIC                     0x5333u   /* "S3" */
#define SLP_VERSION                   1u

#define SLP_CTRL_PORT                 55150u
#define SLP_STREAM_PORT               55151u

#define SLP_LINE_MAX_FRAGMENT_PIXELS  480u      /* 24 + 3*480 = 1464 B < 1472 B (MTU 1500) */
#define SLP_MAX_BUTTONS               4u
#define SLP_MAX_LEDS                  4u
#define SLP_OVERLAY_MAX_ITEMS         3u
#define SLP_OVERLAY_LABEL_LEN         14u
#define SLP_OVERLAY_VALUE_LEN         10u
#define SLP_NAME_LEN                  16u
#define SLP_CFG_MAX_ITEMS             16u

#define SLP_DEFAULT_HID_RATE_HZ       200u
#define SLP_MAX_HID_RATE_HZ           1000u
#define SLP_SESSION_TIMEOUT_MS        3000u
#define SLP_PING_PERIOD_MS            500u
#define SLP_HELLO_PERIOD_MS           1000u
#define SLP_BIND_RETRY_MS             300u

/* Largest CONTROL datagram either side has to buffer. */
#define SLP_CTRL_MAX_BYTES            160u

/* -------------------------------------------------------------------------- */
/* Message types                                                              */
/* -------------------------------------------------------------------------- */
enum slp_msg_type
{
    /* CONTROL, host -> device */
    SLP_HELLO        = 0x01,
    SLP_BIND         = 0x02,
    SLP_UNBIND       = 0x03,
    SLP_PING         = 0x04,
    SLP_LED_SET      = 0x10,
    SLP_OLED_OVERLAY = 0x11,
    SLP_OLED_CLEAR   = 0x12,
    SLP_CFG_GET      = 0x20,
    SLP_CFG_SET      = 0x21,
    SLP_CAL_START    = 0x22,

    /* CONTROL, device -> host */
    SLP_ANNOUNCE     = 0x81,
    SLP_BIND_ACK     = 0x82,
    SLP_PONG         = 0x84,
    SLP_CFG_REPLY    = 0xA0,
    SLP_ERROR        = 0xFF,

    /* STREAM, device -> host */
    SLP_LINE         = 0xC0,
    SLP_HID          = 0xC1
};

enum slp_bind_status
{
    SLP_BIND_OK          = 0,
    SLP_BIND_BUSY        = 1,   /* another peer owns the session */
    SLP_BIND_UNSUPPORTED = 2,   /* protocol version / feature not supported */
    SLP_BIND_BAD_PARAM   = 3
};

enum slp_stream_mode
{
    SLP_STREAM_UNICAST_TO_SENDER = 0,   /* stream to the source address of the BIND */
    SLP_STREAM_MULTICAST         = 1    /* stream to slp_bind.mcast_group */
};

enum slp_hw_family
{
    SLP_HW_CIS = 1
};

enum slp_led_kind  { SLP_LED_MONO_PWM = 0, SLP_LED_RGB = 1 };
enum slp_imu_kind  { SLP_IMU_NONE = 0, SLP_IMU_6AXIS = 1 };
enum slp_cal_kind  { SLP_CAL_CIS = 0, SLP_CAL_IMU = 1 };

enum slp_error_code
{
    SLP_ERR_BAD_VERSION  = 1,
    SLP_ERR_BAD_LENGTH   = 2,
    SLP_ERR_NOT_BOUND    = 3,   /* message needs a session (or wrong session id) */
    SLP_ERR_UNKNOWN_TYPE = 4,
    SLP_ERR_BUSY         = 5,
    SLP_ERR_BAD_PARAM    = 6
};

/* slp_announce.features / slp_bind.want_features */
#define SLP_FEAT_LED_SET       (1u << 0)
#define SLP_FEAT_OLED_OVERLAY  (1u << 1)
#define SLP_FEAT_CFG           (1u << 2)
#define SLP_FEAT_CAL           (1u << 3)
#define SLP_FEAT_HID_BUTTONS   (1u << 8)
#define SLP_FEAT_HID_ACC       (1u << 9)
#define SLP_FEAT_HID_GYRO      (1u << 10)
#define SLP_FEAT_HID_TEMP      (1u << 11)

/* slp_hid.valid_mask / slp_bind_ack.hid_valid_mask (= SLP_FEAT_HID_* >> 8) */
#define SLP_HID_BUTTONS        (1u << 0)
#define SLP_HID_ACC            (1u << 1)
#define SLP_HID_GYRO           (1u << 2)
#define SLP_HID_TEMP           (1u << 3)

/* slp_led_cmd.flags */
#define SLP_LED_NO_LOCAL_PRESS (1u << 0)   /* do not light the LED while its button is pressed */

/* slp_overlay_item.flags */
#define SLP_OVL_BIPOLAR        (1u << 0)   /* draw a centre mark on the bar */
#define SLP_OVL_HIGHLIGHT      (1u << 1)   /* "last touched": full contrast */

/* slp_pong.link_flags */
#define SLP_LINK_STREAMING     (1u << 0)
#define SLP_LINK_CAL_RUNNING   (1u << 1)

/* slp_cfg_item.flags (reply) */
#define SLP_CFG_F_REBOOT       (1u << 0)   /* value stored, takes effect after reboot */
#define SLP_CFG_F_READONLY     (1u << 1)
#define SLP_CFG_F_UNKNOWN      (1u << 2)   /* id not supported by this device */
#define SLP_CFG_F_REJECTED     (1u << 3)   /* out of range */

enum slp_cfg_type
{
    SLP_CFG_U8  = 0,
    SLP_CFG_U16 = 1,
    SLP_CFG_U32 = 2,
    SLP_CFG_F32 = 3,   /* IEEE-754 bit pattern in .value */
    SLP_CFG_IP4 = 4    /* a.b.c.d packed as a | b<<8 | c<<16 | d<<24 */
};

enum slp_cfg_id
{
    SLP_CFG_DPI                 = 1,   /* u16, 200 | 400, reboot */
    SLP_CFG_OVERSAMPLING        = 2,   /* u8, 1..32 */
    SLP_CFG_HANDEDNESS          = 3,   /* u8, 0 | 1 */
    SLP_CFG_GYRO_FS             = 4,   /* u8, 0..7 (recalibrates) */
    SLP_CFG_ACCEL_FS            = 5,   /* u8, 0..3 (recalibrates) */
    SLP_CFG_GUI_SHOW_IMU        = 6,   /* u8, 0 | 1 */
    SLP_CFG_GUI_INVERT          = 7,   /* u8, 0 | 1 */
    SLP_CFG_SCREENSAVER_S       = 8,   /* u16, 1..1000 */
    SLP_CFG_MOTION_THR_ACC      = 9,   /* f32, g */
    SLP_CFG_MOTION_THR_GYRO     = 10,  /* f32, dps */
    SLP_CFG_NET_IP              = 20,  /* ip4, reboot */
    SLP_CFG_NET_MASK            = 21,  /* ip4, reboot */
    SLP_CFG_NET_GW              = 22,  /* ip4, reboot */
    SLP_CFG_NET_DEST_IP         = 23,  /* ip4, unbound-fallback stream target */
    SLP_CFG_STREAM_PORT         = 24,  /* u16, unbound-fallback stream port */
    SLP_CFG_STREAM_WHEN_UNBOUND = 25,  /* u8, 0 | 1 */
    SLP_CFG_LINK_PORT           = 26,  /* u16, reboot */
    SLP_CFG_LINE_RATE           = 40   /* u16, lines/s, read-only */
};

/* -------------------------------------------------------------------------- */
/* Structures (packed, little-endian)                                         */
/* -------------------------------------------------------------------------- */
#pragma pack(push, 1)

struct slp_hdr                      /* 12 B */
{
    uint16_t magic;                 /* SLP_MAGIC */
    uint8_t  version;               /* SLP_VERSION */
    uint8_t  type;                  /* enum slp_msg_type */
    uint16_t length;                /* total datagram length, header included */
    uint16_t flags;                 /* reserved, 0 */
    uint32_t seq;                   /* per sender, per flow */
};

/* ---- CONTROL: host -> device -------------------------------------------- */

struct slp_hello                    /* 20 B */
{
    struct slp_hdr hdr;
    uint8_t  host_version[3];       /* host application version (major, minor, patch) */
    uint8_t  proto_min;             /* lowest protocol version the host accepts */
    uint32_t want_features;         /* SLP_FEAT_* the host intends to use */
};

struct slp_bind                     /* 40 B */
{
    struct slp_hdr hdr;
    uint32_t session;               /* nonce chosen by the host, echoed everywhere */
    uint16_t stream_port;           /* 0 = SLP_STREAM_PORT */
    uint8_t  stream_mode;           /* enum slp_stream_mode */
    uint8_t  reserved0;
    uint8_t  mcast_group[4];        /* SLP_STREAM_MULTICAST only */
    uint16_t hid_rate_hz;           /* 0 = device default */
    uint16_t dpi;                   /* informative; 0 = keep. Use CFG_SET to change. */
    uint32_t want_features;
    uint8_t  host_version[3];
    uint8_t  reserved1[5];
};

struct slp_unbind                   /* 16 B */
{
    struct slp_hdr hdr;
    uint32_t session;
};

struct slp_ping                     /* 20 B */
{
    struct slp_hdr hdr;
    uint32_t session;
    uint32_t host_time_ms;          /* echoed in PONG (round-trip measurement) */
};

struct slp_led_cmd                  /* 12 B - mirrors the device's two-phase LED animation */
{
    uint8_t  brightness_1;          /* 0..100 */
    uint8_t  glide_1;               /* 0..100 % of time_1 spent ramping */
    uint16_t time_1_ms;             /* 0 = hold forever */
    uint8_t  brightness_2;
    uint8_t  glide_2;
    uint16_t time_2_ms;
    uint16_t blink_count;           /* 0 = repeat forever */
    uint8_t  flags;                 /* SLP_LED_* */
    uint8_t  reserved;
};

struct slp_led_set                  /* 64 B */
{
    struct slp_hdr hdr;
    uint8_t  led_mask;              /* bit i = led[i] carries a command */
    uint8_t  reserved[3];
    struct slp_led_cmd led[SLP_MAX_LEDS];
};

struct slp_overlay_item             /* 28 B */
{
    char     label[SLP_OVERLAY_LABEL_LEN];   /* not necessarily NUL-terminated */
    char     value[SLP_OVERLAY_VALUE_LEN];   /* idem */
    uint16_t norm;                  /* 0..65535 bar position, 0xFFFF = no bar */
    uint8_t  flags;                 /* SLP_OVL_* */
    uint8_t  reserved;
};

struct slp_oled_overlay             /* 100 B */
{
    struct slp_hdr hdr;
    uint16_t ttl_ms;                /* overlay disappears this long after the last message */
    uint8_t  count;                 /* 0..SLP_OVERLAY_MAX_ITEMS (0 = clear) */
    uint8_t  layout;                /* 0 = auto */
    struct slp_overlay_item item[SLP_OVERLAY_MAX_ITEMS];
};

struct slp_oled_clear               /* 12 B */
{
    struct slp_hdr hdr;
};

struct slp_cfg_item                 /* 8 B */
{
    uint16_t id;                    /* enum slp_cfg_id */
    uint8_t  type;                  /* enum slp_cfg_type */
    uint8_t  flags;                 /* SLP_CFG_F_* (reply only) */
    uint32_t value;
};

struct slp_cfg_msg                  /* 144 B - CFG_GET (ids only), CFG_SET, CFG_REPLY */
{
    struct slp_hdr hdr;
    uint8_t  count;
    uint8_t  reserved[3];
    struct slp_cfg_item item[SLP_CFG_MAX_ITEMS];
};

struct slp_cal_start                /* 16 B */
{
    struct slp_hdr hdr;
    uint8_t  kind;                  /* enum slp_cal_kind */
    uint8_t  reserved[3];
};

/* ---- CONTROL: device -> host -------------------------------------------- */

struct slp_announce                 /* 112 B */
{
    struct slp_hdr hdr;
    uint8_t  uid[12];               /* MCU unique id */
    uint8_t  mac[6];
    uint8_t  hw_family;             /* enum slp_hw_family */
    uint8_t  hw_revision;
    uint8_t  fw_version[3];         /* major, minor, patch */
    uint8_t  proto_min;
    char     name[SLP_NAME_LEN];    /* "Sp3ctra-XXXX", NUL-padded */
    uint16_t ctrl_port;
    uint16_t stream_port;           /* device default stream port */
    uint8_t  bound;                 /* 1 = a session is active */
    uint8_t  bound_peer_ip[4];
    uint8_t  reserved0[3];
    uint32_t features;              /* SLP_FEAT_* */
    uint8_t  n_buttons;
    uint8_t  n_leds;
    uint8_t  led_kind;              /* enum slp_led_kind */
    uint8_t  imu_kind;              /* enum slp_imu_kind */
    uint16_t display_w;
    uint16_t display_h;
    uint8_t  display_bpp;
    uint8_t  n_dpi;
    uint16_t dpi[4];
    uint16_t pixels_at_dpi[4];
    uint16_t line_rate_max;         /* lines/s */
    uint16_t hid_rate_max;          /* Hz */
    uint8_t  reserved1[14];
};

struct slp_bind_ack                 /* 44 B */
{
    struct slp_hdr hdr;
    uint32_t session;
    uint8_t  status;                /* enum slp_bind_status */
    uint8_t  reserved0[3];
    /* negotiated stream layout - the host configures its parser from THIS */
    uint16_t dpi;
    uint16_t pixels_per_line;
    uint8_t  fragment_count;
    uint8_t  reserved1;
    uint16_t fragment_pixels;       /* pixels per fragment (last one may carry fewer) */
    uint16_t line_packet_bytes;     /* sizeof(slp_line_hdr) + 3 * fragment_pixels */
    uint16_t hid_rate_hz;
    uint16_t hid_valid_mask;        /* SLP_HID_* */
    uint16_t session_timeout_ms;
    uint8_t  reserved2[8];
};

struct slp_pong                     /* 40 B */
{
    struct slp_hdr hdr;
    uint32_t session;
    uint32_t host_time_ms;          /* echoed from PING */
    uint32_t uptime_ms;
    uint32_t lines_sent;
    uint16_t line_rate_lps;
    uint8_t  cal_state;             /* device calibration state machine */
    uint8_t  cal_progress;          /* 0..100 */
    int16_t  temp_c_x10;
    uint8_t  link_flags;            /* SLP_LINK_* */
    uint8_t  reserved[5];
};

struct slp_error                    /* 16 B */
{
    struct slp_hdr hdr;
    uint8_t  in_reply_to;           /* message type that triggered the error */
    uint8_t  code;                  /* enum slp_error_code */
    uint16_t reserved;
};

/* ---- STREAM: device -> host --------------------------------------------- */

struct slp_line_hdr                 /* 24 B, followed by r[n] g[n] b[n], n = pixel_count */
{
    struct slp_hdr hdr;             /* seq = line datagram counter */
    uint32_t line_id;
    uint16_t pixel_offset;          /* first pixel of this fragment */
    uint16_t pixel_count;
    uint8_t  fragment_index;
    uint8_t  fragment_count;
    uint16_t line_period_us;        /* 0 = unknown */
};

#define SLP_LINE_BYTES(pixel_count)  (sizeof(struct slp_line_hdr) + 3u * (pixel_count))

struct slp_hid                      /* 72 B */
{
    struct slp_hdr hdr;
    uint32_t timestamp_us;          /* device monotonic clock */
    uint16_t valid_mask;            /* SLP_HID_* */
    uint8_t  button_count;
    uint8_t  button_state;          /* bit i = button i pressed */
    uint32_t button_seq[SLP_MAX_BUTTONS];   /* +1 on every edge (press or release) */
    float    acc[3];                /* g */
    float    gyro[3];               /* dps */
    float    temp_c;
    uint8_t  reserved[8];
};

#pragma pack(pop)

/* -------------------------------------------------------------------------- */
/* Size guards - identical values on every compiler/platform                 */
/* -------------------------------------------------------------------------- */
#if defined(__cplusplus)
#  define SLP_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#  define SLP_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#  define SLP_STATIC_ASSERT(cond, msg) typedef char slp_static_assert_[(cond) ? 1 : -1]
#endif

SLP_STATIC_ASSERT(sizeof(struct slp_hdr)          == 12,  "slp_hdr size");
SLP_STATIC_ASSERT(sizeof(struct slp_hello)        == 20,  "slp_hello size");
SLP_STATIC_ASSERT(sizeof(struct slp_bind)         == 40,  "slp_bind size");
SLP_STATIC_ASSERT(sizeof(struct slp_unbind)       == 16,  "slp_unbind size");
SLP_STATIC_ASSERT(sizeof(struct slp_ping)         == 20,  "slp_ping size");
SLP_STATIC_ASSERT(sizeof(struct slp_led_cmd)      == 12,  "slp_led_cmd size");
SLP_STATIC_ASSERT(sizeof(struct slp_led_set)      == 64,  "slp_led_set size");
SLP_STATIC_ASSERT(sizeof(struct slp_overlay_item) == 28,  "slp_overlay_item size");
SLP_STATIC_ASSERT(sizeof(struct slp_oled_overlay) == 100, "slp_oled_overlay size");
SLP_STATIC_ASSERT(sizeof(struct slp_oled_clear)   == 12,  "slp_oled_clear size");
SLP_STATIC_ASSERT(sizeof(struct slp_cfg_item)     == 8,   "slp_cfg_item size");
SLP_STATIC_ASSERT(sizeof(struct slp_cfg_msg)      == 144, "slp_cfg_msg size");
SLP_STATIC_ASSERT(sizeof(struct slp_cal_start)    == 16,  "slp_cal_start size");
SLP_STATIC_ASSERT(sizeof(struct slp_announce)     == 112, "slp_announce size");
SLP_STATIC_ASSERT(sizeof(struct slp_bind_ack)     == 44,  "slp_bind_ack size");
SLP_STATIC_ASSERT(sizeof(struct slp_pong)         == 40,  "slp_pong size");
SLP_STATIC_ASSERT(sizeof(struct slp_error)        == 16,  "slp_error size");
SLP_STATIC_ASSERT(sizeof(struct slp_line_hdr)     == 24,  "slp_line_hdr size");
SLP_STATIC_ASSERT(sizeof(struct slp_hid)          == 72,  "slp_hid size");
SLP_STATIC_ASSERT(sizeof(struct slp_cfg_msg)      <= SLP_CTRL_MAX_BYTES, "ctrl buffer");

#ifdef __cplusplus
}
#endif

#endif /* SP3CTRA_LINK_H */
