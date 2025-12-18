/**
 ******************************************************************************
 * @file           : rtpmidi_session.c
 * @brief          : RTP-MIDI Session Management Implementation
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

/* Includes ------------------------------------------------------------------*/
#include "rtpmidi.h"
#include "stm32h7xx_hal.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define RTPMIDI_SIGNATURE       0xFFFF
#define RTPMIDI_CMD_IN          0x494E  // "IN" - Invitation
#define RTPMIDI_CMD_OK          0x4F4B  // "OK" - Accepted
#define RTPMIDI_CMD_BY          0x4259  // "BY" - Goodbye
#define RTPMIDI_CMD_CK          0x434B  // "CK" - Clock sync
#define RTPMIDI_PROTOCOL_VER    0x00000002

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static rtpmidi_session_t g_session = {0};
static rtpmidi_rx_callback_t g_rx_callback = NULL;

/* Private function prototypes -----------------------------------------------*/
static void rtpmidi_send_invitation(void);
static void rtpmidi_send_ok(uint32_t initiator_token, uint32_t remote_ssrc);
static void rtpmidi_send_goodbye(void);
static void rtpmidi_send_clock_sync(void);
static void rtpmidi_handle_control_packet(void *arg, struct udp_pcb *pcb,
                                           struct pbuf *p, const ip_addr_t *addr, u16_t port);
static void rtpmidi_handle_data_packet(void *arg, struct udp_pcb *pcb,
                                        struct pbuf *p, const ip_addr_t *addr, u16_t port);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize RTP-MIDI subsystem
 */
rtpmidi_status_t rtpmidi_init(const char *device_name, ip_addr_t *remote_ip)
{
    memset(&g_session, 0, sizeof(g_session));

    // Generate unique SSRC based on STM32 unique ID
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();
    g_session.ssrc = uid0 ^ uid1 ^ uid2;

    // Copy device name
    strncpy((char*)g_session.device_name, device_name, sizeof(g_session.device_name) - 1);

    // Store remote IP
    ip_addr_copy(g_session.remote_ip, *remote_ip);
    g_session.remote_port_control = RTPMIDI_CONTROL_PORT;
    g_session.remote_port_data = RTPMIDI_DATA_PORT;

    // Create UDP PCBs
    g_session.pcb_control = udp_new();
    g_session.pcb_data = udp_new();

    if (!g_session.pcb_control || !g_session.pcb_data) {
        printf("RTP-MIDI: Failed to create UDP PCBs\n");
        return RTPMIDI_ERROR;
    }

    // Bind control port (5004)
    if (udp_bind(g_session.pcb_control, IP_ADDR_ANY, RTPMIDI_CONTROL_PORT) != ERR_OK) {
        printf("RTP-MIDI: Failed to bind control port %d\n", RTPMIDI_CONTROL_PORT);
        return RTPMIDI_ERROR;
    }

    // Bind data port (5005)
    if (udp_bind(g_session.pcb_data, IP_ADDR_ANY, RTPMIDI_DATA_PORT) != ERR_OK) {
        printf("RTP-MIDI: Failed to bind data port %d\n", RTPMIDI_DATA_PORT);
        return RTPMIDI_ERROR;
    }

    // Register callbacks
    udp_recv(g_session.pcb_control, rtpmidi_handle_control_packet, NULL);
    udp_recv(g_session.pcb_data, rtpmidi_handle_data_packet, NULL);

    g_session.state = RTPMIDI_STATE_IDLE;
    g_session.connection_attempts = 0;

    printf("RTP-MIDI: Initialized '%s' on ports %d/%d, SSRC=0x%08lX\n",
           g_session.device_name, RTPMIDI_CONTROL_PORT, RTPMIDI_DATA_PORT, g_session.ssrc);

    return RTPMIDI_OK;
}

/**
 * @brief Start connection to remote peer
 */
rtpmidi_status_t rtpmidi_connect(void)
{
    if (g_session.state != RTPMIDI_STATE_IDLE) {
        return RTPMIDI_ERROR;
    }

    rtpmidi_send_invitation();
    g_session.state = RTPMIDI_STATE_INVITED;
    g_session.last_invite_tick = HAL_GetTick();
    g_session.connection_attempts = 1;

    printf("RTP-MIDI: Invitation sent to %s\n", ipaddr_ntoa(&g_session.remote_ip));
    return RTPMIDI_OK;
}

/**
 * @brief Disconnect from remote peer
 */
rtpmidi_status_t rtpmidi_disconnect(void)
{
    if (g_session.state == RTPMIDI_STATE_CONNECTED ||
        g_session.state == RTPMIDI_STATE_SYNCHRONIZED) {
        rtpmidi_send_goodbye();
    }

    g_session.state = RTPMIDI_STATE_IDLE;
    g_session.connection_attempts = 0;

    printf("RTP-MIDI: Disconnected\n");
    return RTPMIDI_OK;
}

/**
 * @brief Process RTP-MIDI (call periodically)
 */
rtpmidi_status_t rtpmidi_process(void)
{
    uint32_t now = HAL_GetTick();

    switch (g_session.state) {
        case RTPMIDI_STATE_INVITED:
            // Retry invitation if no response
            if (now - g_session.last_invite_tick > RTPMIDI_INVITE_INTERVAL_MS) {
                if (g_session.connection_attempts < RTPMIDI_MAX_ATTEMPTS) {
                    rtpmidi_send_invitation();
                    g_session.last_invite_tick = now;
                    g_session.connection_attempts++;
                    printf("RTP-MIDI: Retry invitation (%d/%d)\n",
                           g_session.connection_attempts, RTPMIDI_MAX_ATTEMPTS);
                } else {
                    printf("RTP-MIDI: Connection timeout\n");
                    g_session.state = RTPMIDI_STATE_IDLE;
                    return RTPMIDI_TIMEOUT;
                }
            }
            break;

        case RTPMIDI_STATE_CONNECTED:
        case RTPMIDI_STATE_SYNCHRONIZED:
            // Send periodic clock sync
            if (now - g_session.last_sync_tick > RTPMIDI_SYNC_INTERVAL_MS) {
                rtpmidi_send_clock_sync();
                g_session.last_sync_tick = now;
            }
            break;

        default:
            break;
    }

    return RTPMIDI_OK;
}

/**
 * @brief Register callback for incoming MIDI messages
 */
void rtpmidi_register_rx_callback(rtpmidi_rx_callback_t callback)
{
    g_rx_callback = callback;
}

/**
 * @brief Check if RTP-MIDI is connected
 */
uint8_t rtpmidi_is_connected(void)
{
    return (g_session.state == RTPMIDI_STATE_CONNECTED ||
            g_session.state == RTPMIDI_STATE_SYNCHRONIZED) ? 1 : 0;
}

/**
 * @brief Get current session state
 */
rtpmidi_state_t rtpmidi_get_state(void)
{
    return g_session.state;
}

/**
 * @brief Get pointer to session (for internal use by packet module)
 */
rtpmidi_session_t* rtpmidi_get_session(void)
{
    return &g_session;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Send invitation packet
 */
static void rtpmidi_send_invitation(void)
{
    uint8_t packet[128];
    uint8_t *p = packet;

    // Signature (0xFFFF)
    *p++ = 0xFF;
    *p++ = 0xFF;

    // Command 'IN'
    *p++ = 'I';
    *p++ = 'N';

    // Protocol version
    uint32_t version = htonl(RTPMIDI_PROTOCOL_VER);
    memcpy(p, &version, 4);
    p += 4;

    // Initiator token (random)
    g_session.initiator_token = HAL_GetTick() ^ g_session.ssrc;
    uint32_t token = htonl(g_session.initiator_token);
    memcpy(p, &token, 4);
    p += 4;

    // SSRC
    uint32_t ssrc = htonl(g_session.ssrc);
    memcpy(p, &ssrc, 4);
    p += 4;

    // Device name (null-terminated)
    strcpy((char*)p, (char*)g_session.device_name);
    p += strlen((char*)g_session.device_name) + 1;

    // Send via LWIP
    struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, p - packet, PBUF_RAM);
    if (pb) {
        memcpy(pb->payload, packet, p - packet);
        udp_sendto(g_session.pcb_control, pb, &g_session.remote_ip, g_session.remote_port_control);
        pbuf_free(pb);
    }
}

/**
 * @brief Send OK (acceptance) packet
 */
static void rtpmidi_send_ok(uint32_t initiator_token, uint32_t remote_ssrc)
{
    uint8_t packet[128];
    uint8_t *p = packet;

    // Signature
    *p++ = 0xFF;
    *p++ = 0xFF;

    // Command 'OK'
    *p++ = 'O';
    *p++ = 'K';

    // Protocol version
    uint32_t version = htonl(RTPMIDI_PROTOCOL_VER);
    memcpy(p, &version, 4);
    p += 4;

    // Initiator token (echo back)
    uint32_t token = htonl(initiator_token);
    memcpy(p, &token, 4);
    p += 4;

    // Our SSRC
    uint32_t ssrc = htonl(g_session.ssrc);
    memcpy(p, &ssrc, 4);
    p += 4;

    // Device name
    strcpy((char*)p, (char*)g_session.device_name);
    p += strlen((char*)g_session.device_name) + 1;

    // Send
    struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, p - packet, PBUF_RAM);
    if (pb) {
        memcpy(pb->payload, packet, p - packet);
        udp_sendto(g_session.pcb_control, pb, &g_session.remote_ip, g_session.remote_port_control);
        pbuf_free(pb);
    }
}

/**
 * @brief Send goodbye packet
 */
static void rtpmidi_send_goodbye(void)
{
    uint8_t packet[32];
    uint8_t *p = packet;

    // Signature
    *p++ = 0xFF;
    *p++ = 0xFF;

    // Command 'BY'
    *p++ = 'B';
    *p++ = 'Y';

    // Protocol version
    uint32_t version = htonl(RTPMIDI_PROTOCOL_VER);
    memcpy(p, &version, 4);
    p += 4;

    // Initiator token
    uint32_t token = htonl(g_session.initiator_token);
    memcpy(p, &token, 4);
    p += 4;

    // Our SSRC
    uint32_t ssrc = htonl(g_session.ssrc);
    memcpy(p, &ssrc, 4);
    p += 4;

    // Send
    struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, p - packet, PBUF_RAM);
    if (pb) {
        memcpy(pb->payload, packet, p - packet);
        udp_sendto(g_session.pcb_control, pb, &g_session.remote_ip, g_session.remote_port_control);
        pbuf_free(pb);
    }
}

/**
 * @brief Send clock sync packet
 */
static void rtpmidi_send_clock_sync(void)
{
    uint8_t packet[32];
    uint8_t *p = packet;

    // Signature
    *p++ = 0xFF;
    *p++ = 0xFF;

    // Command 'CK'
    *p++ = 'C';
    *p++ = 'K';

    // SSRC
    uint32_t ssrc = htonl(g_session.ssrc);
    memcpy(p, &ssrc, 4);
    p += 4;

    // Count (0 for request)
    *p++ = 0;

    // Padding
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;

    // Timestamps (3x 64-bit, set to current tick for simplicity)
    uint64_t timestamp = HAL_GetTick() * 10;  // Convert to 10kHz
    for (int i = 0; i < 3; i++) {
        uint64_t ts = __builtin_bswap64(timestamp);
        memcpy(p, &ts, 8);
        p += 8;
    }

    // Send
    struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, p - packet, PBUF_RAM);
    if (pb) {
        memcpy(pb->payload, packet, p - packet);
        udp_sendto(g_session.pcb_control, pb, &g_session.remote_ip, g_session.remote_port_control);
        pbuf_free(pb);
    }
}

/**
 * @brief Handle incoming control packets
 */
static void rtpmidi_handle_control_packet(void *arg, struct udp_pcb *pcb,
                                           struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    if (p->len < 8) {
        pbuf_free(p);
        return;
    }

    uint8_t *data = (uint8_t*)p->payload;

    // Check signature
    uint16_t signature = (data[0] << 8) | data[1];
    if (signature != RTPMIDI_SIGNATURE) {
        pbuf_free(p);
        return;
    }

    // Get command
    uint16_t command = (data[2] << 8) | data[3];

    switch (command) {
        case RTPMIDI_CMD_IN:  // Invitation from remote
            if (g_session.state == RTPMIDI_STATE_IDLE) {
                // Extract remote SSRC and token
                uint32_t remote_ssrc, token;
                memcpy(&token, data + 8, 4);
                memcpy(&remote_ssrc, data + 12, 4);
                token = ntohl(token);
                remote_ssrc = ntohl(remote_ssrc);

                g_session.remote_ssrc = remote_ssrc;
                g_session.initiator_token = token;

                // Send OK
                rtpmidi_send_ok(token, remote_ssrc);
                g_session.state = RTPMIDI_STATE_CONNECTED;
                g_session.last_sync_tick = HAL_GetTick();

                printf("RTP-MIDI: Received invitation, sent OK, SSRC=0x%08lX\n", remote_ssrc);
            }
            break;

        case RTPMIDI_CMD_OK:  // Accepted
            if (g_session.state == RTPMIDI_STATE_INVITED) {
                // Extract remote SSRC
                uint32_t remote_ssrc;
                memcpy(&remote_ssrc, data + 12, 4);
                g_session.remote_ssrc = ntohl(remote_ssrc);

                g_session.state = RTPMIDI_STATE_CONNECTED;
                g_session.last_sync_tick = HAL_GetTick();

                printf("RTP-MIDI: Session accepted, SSRC=0x%08lX\n", g_session.remote_ssrc);
            }
            break;

        case RTPMIDI_CMD_BY:  // Goodbye
            g_session.state = RTPMIDI_STATE_IDLE;
            printf("RTP-MIDI: Session closed by remote\n");
            break;

        case RTPMIDI_CMD_CK:  // Clock sync
            if (g_session.state == RTPMIDI_STATE_CONNECTED) {
                g_session.state = RTPMIDI_STATE_SYNCHRONIZED;
            }
            // Could implement full clock sync here if needed
            break;
    }

    pbuf_free(p);
}

/**
 * @brief Handle incoming data packets (MIDI messages)
 */
static void rtpmidi_handle_data_packet(void *arg, struct udp_pcb *pcb,
                                        struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    if (!rtpmidi_is_connected() || !g_rx_callback) {
        pbuf_free(p);
        return;
    }

    if (p->len < 12) {  // Minimum RTP header size
        pbuf_free(p);
        return;
    }

    uint8_t *data = (uint8_t*)p->payload;

    // Parse RTP header
    uint8_t version = (data[0] >> 6) & 0x03;
    uint8_t payload_type = data[1] & 0x7F;

    if (version != 2 || payload_type != 0x61) {  // RTP v2, MIDI payload
        pbuf_free(p);
        return;
    }

    // Skip RTP header (12 bytes minimum)
    uint8_t *midi_data = data + 12;
    uint16_t midi_len = p->len - 12;

    // Skip RTP-MIDI payload header (at least 2 bytes)
    if (midi_len < 2) {
        pbuf_free(p);
        return;
    }

    uint8_t flags = midi_data[0];
    uint8_t len = midi_data[1];
    midi_data += 2;
    midi_len -= 2;

    // Parse MIDI commands
    uint8_t *ptr = midi_data;
    while (ptr < midi_data + midi_len) {
        // Skip delta time (variable length, simplified: assume 1 byte)
        if (*ptr & 0x80) {
            ptr++;  // Extended delta time, skip
            if (ptr >= midi_data + midi_len) break;
        }
        ptr++;  // Skip delta time byte

        if (ptr >= midi_data + midi_len) break;

        // Get MIDI status byte
        uint8_t status = *ptr++;
        if (!(status & 0x80)) break;  // Invalid status

        // Get data bytes based on message type
        uint8_t msg_type = status & 0xF0;
        uint8_t data1 = 0, data2 = 0;

        if (msg_type == MIDI_PROGRAM_CHANGE || msg_type == 0xD0) {
            // 1 data byte
            if (ptr < midi_data + midi_len) {
                data1 = *ptr++;
            }
        } else {
            // 2 data bytes
            if (ptr < midi_data + midi_len) {
                data1 = *ptr++;
            }
            if (ptr < midi_data + midi_len) {
                data2 = *ptr++;
            }
        }

        // Call callback
        g_rx_callback(status, data1, data2);
    }

    pbuf_free(p);
}
