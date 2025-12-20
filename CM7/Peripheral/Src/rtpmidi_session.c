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
#include "lwip/api.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define RTPMIDI_SIGNATURE       0xFFFF
#define RTPMIDI_CMD_IN          0x494E  // "IN" - Invitation
#define RTPMIDI_CMD_OK          0x4F4B  // "OK" - Accepted
#define RTPMIDI_CMD_BY          0x4259  // "BY" - Goodbye
#define RTPMIDI_CMD_CK          0x434B  // "CK" - Clock sync
#define RTPMIDI_CMD_RS          0x5253  // "RS" - Receiver Feedback
#define RTPMIDI_PROTOCOL_VER    0x00000002

/* Private macro -------------------------------------------------------------*/
// Convert 100us ticks to/from system ticks (1ms)
#define US100_TO_TICKS(x)       ((x) / 10)
#define TICKS_TO_US100(x)       ((uint64_t)(x) * 10)

/* Private variables ---------------------------------------------------------*/
static rtpmidi_session_t g_session = {0};
static rtpmidi_rx_callback_t g_rx_callback = NULL;

/* Private function prototypes -----------------------------------------------*/
static void rtpmidi_send_invitation(uint8_t is_control_port);
static void rtpmidi_send_ok(uint32_t initiator_token, uint32_t remote_ssrc, uint8_t is_control_port);
static void rtpmidi_send_goodbye(void);
static void rtpmidi_send_clock_sync(uint8_t count, uint64_t ts1, uint64_t ts2, uint64_t ts3);
static void rtpmidi_send_receiver_feedback(void);
static void rtpmidi_data_recv_thread(void *arg);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize RTP-MIDI subsystem
 */
rtpmidi_status_t rtpmidi_init(const char *device_name, ip_addr_t *remote_ip)
{
    memset(&g_session, 0, sizeof(g_session));

    // Generate unique SSRC based on STM32 unique ID and current tick
    // Adding HAL_GetTick() helps to have different SSRC if we reboot quickly
    // and the host still has the old session active.
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();

    // Force a specific SSRC prefix to ensure it's different from Mac's 0x0527...
    // Using 0x5350... ("SP...") as prefix
    g_session.ssrc = 0x53500000 | ((uid0 ^ uid1 ^ uid2 ^ HAL_GetTick()) & 0x000FFFFF);

    // Initialize sequence number randomly (RFC 3550)
    // Using SSRC and Tick as seed
    g_session.sequence_tx = (uint16_t)(g_session.ssrc ^ HAL_GetTick());

    // Copy device name
    strncpy((char*)g_session.device_name, device_name, sizeof(g_session.device_name) - 1);

    // Store remote IP
    ip_addr_copy(g_session.remote_ip, *remote_ip);
    g_session.remote_port_control = RTPMIDI_CONTROL_PORT;
    g_session.remote_port_data = RTPMIDI_DATA_PORT;

    // Create UDP netconns
    g_session.conn_control = netconn_new(NETCONN_UDP);
    g_session.conn_data = netconn_new(NETCONN_UDP);

    if (!g_session.conn_control || !g_session.conn_data) {
        printf("RTP-MIDI: Failed to create UDP connections\n");
        return RTPMIDI_ERROR;
    }

    // Set receive timeout to non-blocking for control (we poll in rtpmidi_process)
    netconn_set_recvtimeout(g_session.conn_control, 1);

    // Set receive timeout for data connection (blocking with timeout)
    netconn_set_recvtimeout(g_session.conn_data, 100);

    // Bind control port (5004)
    if (netconn_bind(g_session.conn_control, IP_ADDR_ANY, RTPMIDI_CONTROL_PORT) != ERR_OK) {
        printf("RTP-MIDI: Failed to bind control port %d\n", RTPMIDI_CONTROL_PORT);
        return RTPMIDI_ERROR;
    }

    // Bind data port (5005)
    if (netconn_bind(g_session.conn_data, IP_ADDR_ANY, RTPMIDI_DATA_PORT) != ERR_OK) {
        printf("RTP-MIDI: Failed to bind data port %d\n", RTPMIDI_DATA_PORT);
        return RTPMIDI_ERROR;
    }

    // Create receive thread for data (MIDI messages)
    // Increased stack size to handle potential printf/logging
    osThreadDef(rtpmidi_data_rx, rtpmidi_data_recv_thread, osPriorityNormal, 0, 1024);
    osThreadCreate(osThread(rtpmidi_data_rx), NULL);

    g_session.state = RTPMIDI_STATE_IDLE;
    g_session.connection_attempts = 0;
    g_session.clock_offset = 0;

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

    // Step 1: Send invitation on control port
    rtpmidi_send_invitation(1); // 1 = control port
    g_session.state = RTPMIDI_STATE_INVITED;
    g_session.last_invite_tick = HAL_GetTick();
    g_session.connection_attempts = 1;

    printf("RTP-MIDI: Control invitation sent to %s\n", ipaddr_ntoa(&g_session.remote_ip));
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

    // Check for incoming control packets (non-blocking)
    struct netbuf *buf;
    if (netconn_recv(g_session.conn_control, &buf) == ERR_OK) {
        // Process control packet
        uint8_t *data = (uint8_t*)buf->p->payload;
        uint16_t len = buf->p->len;

        if (len >= 8) {
            // Check signature
            uint16_t signature = (data[0] << 8) | data[1];
            if (signature == RTPMIDI_SIGNATURE) {
                // Get command
                uint16_t command = (data[2] << 8) | data[3];

                switch (command) {
                    case RTPMIDI_CMD_IN:  // Invitation from remote
                        // Accept invitations in IDLE or INVITED state
                        if (g_session.state == RTPMIDI_STATE_IDLE ||
                            g_session.state == RTPMIDI_STATE_INVITED) {
                            // Extract remote SSRC and token
                            uint32_t remote_ssrc, token;
                            memcpy(&token, data + 8, 4);
                            memcpy(&remote_ssrc, data + 12, 4);
                            token = ntohl(token);
                            remote_ssrc = ntohl(remote_ssrc);

                            g_session.remote_ssrc = remote_ssrc;
                            g_session.initiator_token = token;

                            // IMPORTANT: Do NOT overwrite our own SSRC with remote SSRC
                            // g_session.ssrc must remain unique to us

                            // Send OK on control port
                            rtpmidi_send_ok(token, remote_ssrc, 1);

                            // Now we wait for invitation on data port
                            g_session.state = RTPMIDI_STATE_CONTROL_CONNECTED;

                            printf("RTP-MIDI: Control invitation received, sent OK, SSRC=0x%08lX\n", remote_ssrc);
                        }
                        break;

                    case RTPMIDI_CMD_OK:  // Accepted
                        if (g_session.state == RTPMIDI_STATE_INVITED) {
                            // Extract remote SSRC
                            uint32_t remote_ssrc;
                            memcpy(&remote_ssrc, data + 12, 4);
                            g_session.remote_ssrc = ntohl(remote_ssrc);

                            // Control port accepted, now invite on data port
                            g_session.state = RTPMIDI_STATE_CONTROL_CONNECTED;
                            rtpmidi_send_invitation(0); // 0 = data port
                            g_session.last_invite_tick = HAL_GetTick();
                            g_session.connection_attempts = 1;

                            printf("RTP-MIDI: Control accepted, inviting on data port\n");
                        }
                        break;

                    case RTPMIDI_CMD_BY:  // Goodbye
                        g_session.state = RTPMIDI_STATE_IDLE;
                        printf("RTP-MIDI: Session closed by remote\n");
                        break;

                    case RTPMIDI_CMD_RS: // Receiver Feedback
                        // Update last received sequence number from remote
                        // Not critical for now, but good to know remote is alive
                        break;
                }
            }
        }
        netbuf_delete(buf);
    }

    // Check for incoming data packets (for handshake and sync)
    // Note: Normal MIDI data is handled in separate thread, but we need to check for
    // IN/OK/CK commands on data port here or in the thread.
    // Since netconn_recv is blocking/timeout in the thread, we should handle command packets there
    // and update state, or use a separate non-blocking check here if possible.
    // For now, let's assume the data thread handles MIDI and we need to handle commands there too.
    // BUT: The data thread is designed for stream processing.
    // Let's modify the data thread to handle commands as well.

    // Handle session state machine timeouts and retries
    switch (g_session.state) {
        case RTPMIDI_STATE_INVITED:
            // Retry control invitation
            if (now - g_session.last_invite_tick > RTPMIDI_INVITE_INTERVAL_MS) {
                if (g_session.connection_attempts < RTPMIDI_MAX_ATTEMPTS) {
                    rtpmidi_send_invitation(1); // Control port
                    g_session.last_invite_tick = now;
                    g_session.connection_attempts++;
                    printf("RTP-MIDI: Retry control invitation (%d/%d)\n",
                           g_session.connection_attempts, RTPMIDI_MAX_ATTEMPTS);
                } else {
                    printf("RTP-MIDI: Connection timeout (control)\n");
                    g_session.state = RTPMIDI_STATE_IDLE;
                    return RTPMIDI_TIMEOUT;
                }
            }
            break;

        case RTPMIDI_STATE_CONTROL_CONNECTED:
            // Retry data invitation if we are the initiator
            // If we are responder, we wait for IN on data port
            if (g_session.connection_attempts > 0) { // We are initiator
                if (now - g_session.last_invite_tick > RTPMIDI_INVITE_INTERVAL_MS) {
                    if (g_session.connection_attempts < RTPMIDI_MAX_ATTEMPTS) {
                        rtpmidi_send_invitation(0); // Data port
                        g_session.last_invite_tick = now;
                        g_session.connection_attempts++;
                        printf("RTP-MIDI: Retry data invitation (%d/%d)\n",
                               g_session.connection_attempts, RTPMIDI_MAX_ATTEMPTS);
                    } else {
                        printf("RTP-MIDI: Connection timeout (data)\n");
                        g_session.state = RTPMIDI_STATE_IDLE;
                        return RTPMIDI_TIMEOUT;
                    }
                }
            }
            break;

        case RTPMIDI_STATE_CONNECTED:
            // Force synchronization after timeout if we never received count=2
            if (now - g_session.last_sync_tick > 10000) { // 10 seconds timeout
                printf("RTP-MIDI: Forcing synchronization (timeout)\n");
                g_session.state = RTPMIDI_STATE_SYNCHRONIZED;
                g_session.clock_offset = 0; // No offset
            }
            // Fall through to SYNCHRONIZED case

        case RTPMIDI_STATE_SYNCHRONIZED:
            // Send periodic clock sync
            if (now - g_session.last_sync_tick > RTPMIDI_SYNC_INTERVAL_MS) {
                // Start sync sequence: count=0, ts1=now
                rtpmidi_send_clock_sync(0, TICKS_TO_US100(now), 0, 0);
                g_session.last_sync_tick = now;
            }

            // Send periodic receiver feedback
            if (now - g_session.last_feedback_tick > RTPMIDI_FEEDBACK_INTERVAL_MS) {
                rtpmidi_send_receiver_feedback();
                g_session.last_feedback_tick = now;
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
static void rtpmidi_send_invitation(uint8_t is_control_port)
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

    // Initiator token
    // If starting new session, generate token. If continuing (data port), use existing.
    if (is_control_port && g_session.state == RTPMIDI_STATE_IDLE) {
        g_session.initiator_token = HAL_GetTick() ^ g_session.ssrc;
    }
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

    // Send via netconn
    struct netbuf *buf = netbuf_new();
    if (buf) {
        void *data = netbuf_alloc(buf, p - packet);
        if (data) {
            memcpy(data, packet, p - packet);
            if (is_control_port) {
                netconn_sendto(g_session.conn_control, buf, &g_session.remote_ip, g_session.remote_port_control);
            } else {
                netconn_sendto(g_session.conn_data, buf, &g_session.remote_ip, g_session.remote_port_data);
            }
        }
        netbuf_delete(buf);
    }
}

/**
 * @brief Send OK (acceptance) packet
 */
static void rtpmidi_send_ok(uint32_t initiator_token, uint32_t remote_ssrc, uint8_t is_control_port)
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
    struct netbuf *buf = netbuf_new();
    if (buf) {
        void *data = netbuf_alloc(buf, p - packet);
        if (data) {
            memcpy(data, packet, p - packet);
            if (is_control_port) {
                netconn_sendto(g_session.conn_control, buf, &g_session.remote_ip, g_session.remote_port_control);
            } else {
                netconn_sendto(g_session.conn_data, buf, &g_session.remote_ip, g_session.remote_port_data);
            }
        }
        netbuf_delete(buf);
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
    struct netbuf *buf = netbuf_new();
    if (buf) {
        void *data = netbuf_alloc(buf, p - packet);
        if (data) {
            memcpy(data, packet, p - packet);
            netconn_sendto(g_session.conn_control, buf, &g_session.remote_ip, g_session.remote_port_control);
        }
        netbuf_delete(buf);
    }
}

/**
 * @brief Send clock sync packet
 */
static void rtpmidi_send_clock_sync(uint8_t count, uint64_t ts1, uint64_t ts2, uint64_t ts3)
{
    uint8_t packet[64];
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

    // Count
    *p++ = count;

    // Padding
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;

    // Timestamps (64-bit, network byte order)
    uint64_t ts;

    // Timestamp 1 (always present)
    ts = __builtin_bswap64(ts1);
    memcpy(p, &ts, 8);
    p += 8;

    // Timestamp 2 (if count >= 1)
    if (count >= 1) {
        ts = __builtin_bswap64(ts2);
        memcpy(p, &ts, 8);
        p += 8;
    }

    // Timestamp 3 (if count >= 2)
    if (count >= 2) {
        ts = __builtin_bswap64(ts3);
        memcpy(p, &ts, 8);
        p += 8;
    }

    // Send on control port (Note: Apple spec says CK on MIDI port, but some implementations use control.
    // Let's stick to control port for now as per previous implementation, but verify later)
    // Correction: Apple spec says "Synchronization packets are exchanged between the participants' MIDI ports."
    // So we should send on data port!

    struct netbuf *buf = netbuf_new();
    if (buf) {
        void *data = netbuf_alloc(buf, p - packet);
        if (data) {
            memcpy(data, packet, p - packet);
            netconn_sendto(g_session.conn_data, buf, &g_session.remote_ip, g_session.remote_port_data);
        }
        netbuf_delete(buf);
    }
}

static void rtpmidi_send_receiver_feedback(void)
{
    uint8_t packet[32];
    uint8_t *p = packet;

    // Signature
    *p++ = 0xFF;
    *p++ = 0xFF;

    // Command 'RS'
    *p++ = 'R';
    *p++ = 'S';

    // SSRC
    uint32_t ssrc = htonl(g_session.ssrc);
    memcpy(p, &ssrc, 4);
    p += 4;

    // Sequence number (last received)
    uint16_t seq = htons(g_session.sequence_rx_last);
    memcpy(p, &seq, 2);
    p += 2;

    // Padding
    *p++ = 0;
    *p++ = 0;

    // Send on control port
    struct netbuf *buf = netbuf_new();
    if (buf) {
        void *data = netbuf_alloc(buf, p - packet);
        if (data) {
            memcpy(data, packet, p - packet);
            netconn_sendto(g_session.conn_control, buf, &g_session.remote_ip, g_session.remote_port_control);
        }
        netbuf_delete(buf);
    }
}

/**
 * @brief Thread to receive MIDI data packets
 */
static void rtpmidi_data_recv_thread(void *arg)
{
    struct netbuf *buf;

    for (;;) {
        // Wait for incoming data packet (blocking with timeout)
        if (netconn_recv(g_session.conn_data, &buf) == ERR_OK) {
            uint8_t *data = (uint8_t*)buf->p->payload;
            uint16_t len = buf->p->len;

            // Check for Apple-MIDI command packet (0xFFFF signature)
            if (len >= 4 && data[0] == 0xFF && data[1] == 0xFF) {
                uint16_t command = (data[2] << 8) | data[3];

                switch (command) {
                    case RTPMIDI_CMD_IN: // Invitation on data port
                        if (g_session.state == RTPMIDI_STATE_CONTROL_CONNECTED) {
                            // Extract remote SSRC and token
                            uint32_t remote_ssrc, token;
                            memcpy(&token, data + 8, 4);
                            memcpy(&remote_ssrc, data + 12, 4);
                            token = ntohl(token);
                            remote_ssrc = ntohl(remote_ssrc);

                            // Verify it matches control port session
                            if (remote_ssrc == g_session.remote_ssrc) {
                                // Send OK on data port
                                rtpmidi_send_ok(token, remote_ssrc, 0);
                                g_session.state = RTPMIDI_STATE_CONNECTED;
                                g_session.last_sync_tick = HAL_GetTick();
                                printf("RTP-MIDI: Data invitation received, sent OK. Session CONNECTED.\n");
                            }
                        }
                        break;

                    case RTPMIDI_CMD_OK: // Accepted on data port
                        if (g_session.state == RTPMIDI_STATE_CONTROL_CONNECTED) {
                            g_session.state = RTPMIDI_STATE_CONNECTED;
                            g_session.last_sync_tick = HAL_GetTick();
                            printf("RTP-MIDI: Data invitation accepted. Session CONNECTED.\n");
                        }
                        break;

                    case RTPMIDI_CMD_CK: // Clock Sync
                        printf("RTP-MIDI: Received CK packet, len=%d\n", len);
                        if (len >= 12) { // Minimum size for CK packet (header + count)
                            uint8_t count = data[8];
                            printf("RTP-MIDI: CK count=%d\n", count);
                            uint64_t ts1 = 0, ts2 = 0, ts3 = 0;
                            uint64_t now_us = TICKS_TO_US100(HAL_GetTick());

                            // Extract timestamps
                            memcpy(&ts1, data + 12, 8);
                            ts1 = __builtin_bswap64(ts1);
                            printf("RTP-MIDI: CK ts1=%llu\n", ts1);

                            if (count == 0) {
                                // Initiator sent count=0, we respond with count=1
                                // We store ts1 as the remote initiator time
                                printf("RTP-MIDI: Responding to count=0 with count=1\n");
                                rtpmidi_send_clock_sync(1, ts1, now_us, 0);
                            } else if (count == 1) {
                                // Initiator sent count=1, we respond with count=2
                                if (len >= 28) { // Need ts2
                                    memcpy(&ts2, data + 20, 8);
                                    ts2 = __builtin_bswap64(ts2);
                                    printf("RTP-MIDI: Responding to count=1 with count=2, ts2=%llu\n", ts2);
                                    // ts1 = remote time at count 0
                                    // ts2 = our time at count 1
                                    // now_us = our time at count 2
                                    rtpmidi_send_clock_sync(2, ts1, ts2, now_us);
                                } else {
                                    printf("RTP-MIDI: CK count=1 packet too short (%d < 28)\n", len);
                                }
                            } else if (count == 2) {
                                // Sync complete
                                if (len >= 36) { // Need ts2 and ts3
                                    memcpy(&ts2, data + 20, 8);
                                    memcpy(&ts3, data + 28, 8);
                                    ts2 = __builtin_bswap64(ts2);
                                    ts3 = __builtin_bswap64(ts3);
                                    printf("RTP-MIDI: Processing count=2, ts2=%llu, ts3=%llu\n", ts2, ts3);

                                    // Calculate offset: offset = (ts1 + ts3)/2 - ts2
                                    // Note: ts1/ts3 are remote time, ts2 is local time
                                    // This offset allows us to convert our local time to remote time
                                    int64_t offset = (int64_t)((ts1 + ts3) / 2) - (int64_t)ts2;
                                    g_session.clock_offset = offset;
                                    g_session.state = RTPMIDI_STATE_SYNCHRONIZED;
                                    printf("RTP-MIDI: Sync complete. Offset=%lld\n", offset);
                                } else {
                                    printf("RTP-MIDI: CK count=2 packet too short (%d < 36)\n", len);
                                }
                            } else {
                                printf("RTP-MIDI: Unknown CK count=%d\n", count);
                            }
                        } else {
                            printf("RTP-MIDI: CK packet too short (%d < 12)\n", len);
                        }
                        break;
                }
            }
            // Check for RTP-MIDI data packet
            else if (rtpmidi_is_connected() && g_rx_callback && len >= 12) {
                // Parse RTP header
                uint8_t version = (data[0] >> 6) & 0x03;
                uint8_t payload_type = data[1] & 0x7F;
                uint16_t seq = (data[2] << 8) | data[3];

                if (version == 2 && payload_type == 0x61) {  // RTP v2, MIDI payload
                    // Update last received sequence number
                    g_session.sequence_rx_last = seq;

                    // Skip RTP header (12 bytes minimum)
                    uint8_t *midi_data = data + 12;
                    uint16_t midi_len = len - 12;

                    // Skip RTP-MIDI payload header (at least 2 bytes)
                    if (midi_len >= 2) {
                        // Skip flags and length bytes
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
                    }
                }
            }
            netbuf_delete(buf);
        }
    }
}
