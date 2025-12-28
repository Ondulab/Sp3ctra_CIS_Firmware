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

// RTP (RFC3550) / RTP-MIDI (RFC6295)
// Keep local define here to avoid header coupling.
#define RTP_PAYLOAD_TYPE_MIDI   0x61U

/* Private macro -------------------------------------------------------------*/
// Convert 100us ticks to/from system ticks (1ms)
// AppleMIDI timestamps are in 10kHz units (100µs resolution)
// HAL_GetTick() returns milliseconds
// 1 ms = 10 units of 100µs, so multiply by 10
#define US100_TO_TICKS(x)       ((x) / 10)
#define TICKS_TO_US100(x)       ((uint64_t)(x) * 10ULL)

/* Private variables ---------------------------------------------------------*/
static rtpmidi_session_t g_session = {0};
static rtpmidi_rx_callback_t g_rx_callback = NULL;

/* Private function prototypes -----------------------------------------------*/
static void rtpmidi_send_invitation(uint8_t is_control_port);
static void rtpmidi_send_ok(uint32_t initiator_token, uint32_t remote_ssrc, uint8_t is_control_port);
static void rtpmidi_send_goodbye(void);
static void rtpmidi_send_clock_sync(uint8_t count, uint64_t ts1, uint64_t ts2, uint64_t ts3);
// Reserved for future use (RS implementation), keep declaration commented to avoid warnings.
// static void rtpmidi_send_receiver_feedback_with_seq(uint16_t seq);
static void rtpmidi_data_recv_thread(void *arg);

static void rtpmidi_handle_clock_sync(const uint8_t *data, uint16_t len);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize RTP-MIDI subsystem
 */
rtpmidi_status_t rtpmidi_init(const char *device_name, ip_addr_t *remote_ip, rtpmidi_mode_t mode)
{
    memset(&g_session, 0, sizeof(g_session));

    // Store operation mode
    g_session.mode = mode;

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

    // Store remote IP (only used in CLIENT mode)
    if (remote_ip) {
        ip_addr_copy(g_session.remote_ip, *remote_ip);
    }
    g_session.remote_port_control = RTPMIDI_CONTROL_PORT;
    g_session.remote_port_data = RTPMIDI_DATA_PORT;

    // Peer ports are learned from incoming packets (macOS may use ephemeral source ports).
    g_session.peer_port_control = 0;
    g_session.peer_port_data = 0;

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
    g_session.ck_sync_initiated = 0;

    printf("RTP-MIDI: Initialized '%s' on ports %d/%d, SSRC=0x%08lX\n",
           g_session.device_name, RTPMIDI_CONTROL_PORT, RTPMIDI_DATA_PORT, g_session.ssrc);

    return RTPMIDI_OK;
}

/**
 * @brief Start connection to remote peer (CLIENT mode only)
 */
rtpmidi_status_t rtpmidi_connect(void)
{
    // In SERVER mode, we never initiate connections
    if (g_session.mode == RTPMIDI_MODE_SERVER) {
        printf("RTP-MIDI: Cannot connect in SERVER mode (passive mode)\n");
        return RTPMIDI_ERROR;
    }

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
    g_session.ck_sync_initiated = 0;  // Reset for next connection

    printf("RTP-MIDI: Disconnected\n");
    return RTPMIDI_OK;
}

/**
 * @brief Process RTP-MIDI (call periodically)
 */
rtpmidi_status_t rtpmidi_process(void)
{
    uint32_t now = HAL_GetTick();

    // CRITICAL: Process ALL pending control packets BEFORE handling retries
    // This ensures OK packets are processed before retry logic runs
    struct netbuf *buf;
    uint8_t packet_received = 0;  // Track if we received any packet this cycle
    while (netconn_recv(g_session.conn_control, &buf) == ERR_OK) {
        packet_received = 1;  // Mark that we received a packet
        // Always track the source of incoming control packets.
        // This is required to correctly reply to macOS-initiated invitations (IN) even in CLIENT mode.
        ip_addr_copy(g_session.remote_ip, *netbuf_fromaddr(buf));
        g_session.peer_port_control = netbuf_fromport(buf);

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
                        // Note: macOS will retry every 1s and eventually send BY if it doesn't receive OK.
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

                            // Update state BEFORE sending OK to avoid race condition
                            // This ensures the data thread is ready to accept INVITE before macOS receives our OK
                            g_session.state = RTPMIDI_STATE_CONTROL_CONNECTED;

                            // CRITICAL: Stop retry timer by clearing connection_attempts
                            // This prevents sending additional invitations after receiving remote's INVITE
                            g_session.connection_attempts = 0;

                            // Send OK on control port
                            rtpmidi_send_ok(token, remote_ssrc, 1);

                            printf("RTP-MIDI: Control invitation received, sent OK, SSRC=0x%08lX\n", remote_ssrc);
                        }
                        break;

                    case RTPMIDI_CMD_OK:  // Accepted
                        if (g_session.state == RTPMIDI_STATE_INVITED) {
                            // Extract remote SSRC
                            uint32_t remote_ssrc;
                            memcpy(&remote_ssrc, data + 12, 4);
                            g_session.remote_ssrc = ntohl(remote_ssrc);

                            // CRITICAL: Change state FIRST to stop retry immediately
                            // The retry logic checks state before sending, so this must be first
                            g_session.state = RTPMIDI_STATE_CONTROL_CONNECTED;

                            // Then stop retry counter (belt and suspenders)
                            g_session.connection_attempts = 0;

                            // Now safe to send data port invitation
                            rtpmidi_send_invitation(0); // 0 = data port
                            g_session.last_invite_tick = HAL_GetTick();
                            g_session.connection_attempts = 1;  // Start data port retry counter

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

    // Handle session state machine timeouts and retries (CLIENT mode only)
    // In SERVER mode, we never initiate or retry invitations
    // CRITICAL: Skip retry if we just received a packet (state may have changed)
    if (g_session.mode == RTPMIDI_MODE_CLIENT && !packet_received) {
        switch (g_session.state) {
            case RTPMIDI_STATE_INVITED:
                // Limited retry: max 3 attempts total (initial + 2 retries)
                // Check state again to avoid race with OK handler
                if (g_session.state == RTPMIDI_STATE_INVITED &&
                    now - g_session.last_invite_tick > RTPMIDI_INVITE_INTERVAL_MS) {
                    if (g_session.connection_attempts < 3) {  // Max 3 attempts total
                        rtpmidi_send_invitation(1); // Control port
                        g_session.last_invite_tick = now;
                        g_session.connection_attempts++;
                        printf("RTP-MIDI: Retry control invitation (%d/3)\n",
                               g_session.connection_attempts);
                    } else {
                        printf("RTP-MIDI: Connection timeout (control) - no response after 3 attempts\n");
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

            default:
                break;
        }
    }

    // Handle connected/synchronized states (both modes)
    switch (g_session.state) {

        case RTPMIDI_STATE_CONNECTED:
            // Force synchronization after timeout if we never received count=2
            if (now - g_session.last_sync_tick > 10000) { // 10 seconds timeout
                printf("RTP-MIDI: Forcing synchronization (timeout)\n");
                g_session.state = RTPMIDI_STATE_SYNCHRONIZED;
                g_session.clock_offset = 0; // No offset
            }
            // Fall through to SYNCHRONIZED case
            __attribute__((fallthrough));

        case RTPMIDI_STATE_SYNCHRONIZED:
            // CLIENT mode: Send periodic clock sync (CK) to keep session alive
            // This is REQUIRED for macOS to accept MIDI data from the initiator
            // Linux rtpmidi sends CK every ~2 seconds as the initiator
            if (g_session.mode == RTPMIDI_MODE_CLIENT) {
                if (now - g_session.last_sync_tick > 1500) { // Every 1.5 seconds like macOS
                    uint64_t now_100us = TICKS_TO_US100(now);
                    rtpmidi_send_clock_sync(0U, now_100us, 0ULL, 0ULL);
                    g_session.last_sync_tick = now;
                }
            }
            // SERVER mode: We respond to CK from the initiator (handled in data thread)
            // No need to send periodic CK ourselves

            // Receiver Feedback (RS): do not send periodic keepalive.
            // macOS will send RS to us, but we don't need to send it back periodically.
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
    // Guard: Do not send invitations if we're NOT in the correct state
    // This prevents race conditions where we might send an invitation after receiving OK
    if (is_control_port) {
        // For control port, only send if in IDLE or INVITED state
        if (g_session.state != RTPMIDI_STATE_IDLE &&
            g_session.state != RTPMIDI_STATE_INVITED) {
            printf("RTP-MIDI: Skipping control invitation - wrong state %d\n", g_session.state);
            return;
        }
    } else {
        // For data port, only send if in CONTROL_CONNECTED state
        if (g_session.state != RTPMIDI_STATE_CONTROL_CONNECTED) {
            printf("RTP-MIDI: Skipping data invitation - wrong state %d\n", g_session.state);
            return;
        }
    }

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
                netconn_sendto(g_session.conn_control, buf, &g_session.remote_ip,
                               (g_session.peer_port_control != 0U) ? g_session.peer_port_control : g_session.remote_port_control);
            } else {
                netconn_sendto(g_session.conn_data, buf, &g_session.remote_ip,
                               (g_session.peer_port_data != 0U) ? g_session.peer_port_data : g_session.remote_port_data);
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

    // Debug: log destination used to send OK (helps diagnose macOS-initiated IN not being acknowledged)
    printf("RTP-MIDI: Sending OK on %s to %s:%u (remote_ssrc=0x%08lX)\n",
           is_control_port ? "control" : "data",
           ipaddr_ntoa(&g_session.remote_ip),
           (unsigned)(is_control_port ? g_session.peer_port_control : g_session.peer_port_data),
           (unsigned long)remote_ssrc);

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
    // IMPORTANT: Reply to the actual UDP source port used by the peer.
    // macOS may initiate AppleMIDI from an ephemeral source port (not 5004/5005),
    // and expects the response on that same port.
    struct netbuf *buf = netbuf_new();
    if (buf) {
        void *data = netbuf_alloc(buf, p - packet);
        if (data) {
            memcpy(data, packet, p - packet);
            if (is_control_port) {
                netconn_sendto(g_session.conn_control, buf, &g_session.remote_ip,
                               (g_session.peer_port_control != 0U) ? g_session.peer_port_control : g_session.remote_port_control);
            } else {
                netconn_sendto(g_session.conn_data, buf, &g_session.remote_ip,
                               (g_session.peer_port_data != 0U) ? g_session.peer_port_data : g_session.remote_port_data);
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
            netconn_sendto(g_session.conn_control, buf, &g_session.remote_ip,
                           (g_session.peer_port_control != 0U) ? g_session.peer_port_control : g_session.remote_port_control);
        }
        netbuf_delete(buf);
    }
}

/**
 * @brief Send clock sync packet
 *
 * CK packet format (RFC 6295):
 * - Signature (2 bytes): 0xFFFF
 * - Command (2 bytes): "CK"
 * - SSRC (4 bytes): Sender SSRC
 * - Count (1 byte): Sync sequence number (0, 1, or 2)
 * - Padding (3 bytes): zeros
 * - Timestamp1 (8 bytes): always present
 * - Timestamp2 (8 bytes): present if count >= 1
 * - Timestamp3 (8 bytes): present if count >= 2
 *
 * NOTE: CK packets do NOT have a version field (unlike IN/OK/BY packets)!
 */
static void rtpmidi_send_clock_sync(uint8_t count, uint64_t ts1, uint64_t ts2, uint64_t ts3)
{
    uint8_t packet[64] = {0};  // Initialize to zero to avoid sending garbage data
    uint8_t *p = packet;

    // Debug: print timestamps
    printf("RTP-MIDI: Sending CK count=%d, ts1=%llu, ts2=%llu, ts3=%llu\n",
           count, (unsigned long long)ts1, (unsigned long long)ts2, (unsigned long long)ts3);

    // Signature
    *p++ = 0xFF;
    *p++ = 0xFF;

    // Command 'CK'
    *p++ = 'C';
    *p++ = 'K';

    // SSRC (NO VERSION FIELD for CK packets!)
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
    // IMPORTANT: Apple MIDI expects ALL 3 timestamps to be present (36 bytes total)
    // even for count=0. Timestamps not yet filled should be zero.
    uint64_t ts;

    // Timestamp 1 (always present)
    ts = __builtin_bswap64(ts1);
    memcpy(p, &ts, 8);
    p += 8;

    // Timestamp 2 (always present - zero if count < 1)
    ts = __builtin_bswap64(ts2);
    memcpy(p, &ts, 8);
    p += 8;

    // Timestamp 3 (always present - zero if count < 2)
    ts = __builtin_bswap64(ts3);
    memcpy(p, &ts, 8);
    p += 8;

    // Send on control port (Note: Apple spec says CK on MIDI port, but some implementations use control.
    // Let's stick to control port for now as per previous implementation, but verify later)
    // Correction: Apple spec says "Synchronization packets are exchanged between the participants' MIDI ports."
    // So we should send on data port!

    struct netbuf *buf = netbuf_new();
    if (buf) {
        void *data = netbuf_alloc(buf, p - packet);
        if (data) {
            memcpy(data, packet, p - packet);
            netconn_sendto(g_session.conn_data, buf, &g_session.remote_ip,
                           (g_session.peer_port_data != 0U) ? g_session.peer_port_data : g_session.remote_port_data);
        }
        netbuf_delete(buf);
    }
}



/**
 * @brief Handle AppleMIDI Clock Synchronization (CK) on data port.
 *
 * Implements the 3-step exchange described in Apple's MIDI Network Driver Protocol.
 * - Receive count=0 => send count=1 (copy t1, set t2)
 * - Receive count=1 => send count=2 (copy t1,t2, set t3)
 *
 * NOTE:
 * - Timestamps are 64-bit in units of 100us.
 * - We use local time derived from HAL_GetTick() (ms) converted to 100us units.
 * - No dynamic allocation.
 */
static void rtpmidi_handle_clock_sync(const uint8_t *data, uint16_t len)
{
    // Expected minimum size for count=0:
    // 0xFFFF + 'CK' + SSRC(4) + count(1) + pad(3) + ts1(8) = 22 bytes
    if (len < 22U) {
        return;
    }

    // Layout after signature+command:
    // [4..7]   SSRC
    // [8]      count
    // [9..11]  pad
    // [12..]   timestamps
    uint8_t count = data[8];

    // Parse ts1
    uint64_t ts1 = 0;
    memcpy(&ts1, data + 12, 8);
    ts1 = __builtin_bswap64(ts1);

    // Local time in 100us units
    uint64_t now_100us = TICKS_TO_US100(HAL_GetTick());

    if (count == 0U) {
        // Respond with count=1, copy ts1, set ts2=now
        rtpmidi_send_clock_sync(1U, ts1, now_100us, 0ULL);

        // Consider session synchronized once the sync exchange starts.
        if (g_session.state == RTPMIDI_STATE_CONNECTED) {
            g_session.state = RTPMIDI_STATE_SYNCHRONIZED;
        }
        return;
    }

    if (count == 1U) {
        // Need ts2 present
        if (len < (22U + 8U)) {
            return;
        }
        uint64_t ts2 = 0;
        memcpy(&ts2, data + 20, 8);
        ts2 = __builtin_bswap64(ts2);

        // Respond with count=2, copy ts1 & ts2, set ts3=now
        rtpmidi_send_clock_sync(2U, ts1, ts2, now_100us);

        if (g_session.state == RTPMIDI_STATE_CONNECTED) {
            g_session.state = RTPMIDI_STATE_SYNCHRONIZED;
        }
        return;
    }

    // count=2 received: sync complete. We don't need to respond.
    if (count == 2U) {
        if (g_session.state == RTPMIDI_STATE_CONNECTED) {
            g_session.state = RTPMIDI_STATE_SYNCHRONIZED;
        }
        return;
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

            // Always track the source of incoming data packets.
            // Required to reply to macOS-initiated invitations on the MIDI data port.
            ip_addr_copy(g_session.remote_ip, *netbuf_fromaddr(buf));
            g_session.peer_port_data = netbuf_fromport(buf);

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
                                // CRITICAL: Stop retry timer by clearing connection_attempts
                                // This prevents sending additional invitations after receiving remote's INVITE
                                g_session.connection_attempts = 0;

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

                            // CLIENT mode: Initiate clock sync (CK) exchange
                            // We are the initiator, so we must send CK count=0 first
                            if (g_session.mode == RTPMIDI_MODE_CLIENT && !g_session.ck_sync_initiated) {
                                g_session.ck_sync_initiated = 1;
                                uint64_t now_100us = TICKS_TO_US100(HAL_GetTick());
                                printf("RTP-MIDI: Initiating clock sync (CK count=0)\n");
                                rtpmidi_send_clock_sync(0U, now_100us, 0ULL, 0ULL);
                            }
                        }
                        break;

                    case RTPMIDI_CMD_CK: // Clock Sync
                        // Implement minimal Clock Sync responder to keep macOS session alive.
                        rtpmidi_handle_clock_sync(data, len);
                        break;
                }
            }
            // Check for RTP-MIDI data packet
            else if (rtpmidi_is_connected() && g_rx_callback && len >= 12) {
                // Parse RTP header (RFC3550)
                uint8_t version = (data[0] >> 6) & 0x03;
                uint8_t payload_type = data[1] & 0x7F;
                uint16_t seq = (uint16_t)((data[2] << 8) | data[3]);

                if (version == 2 && payload_type == RTP_PAYLOAD_TYPE_MIDI) {  // RTP v2, MIDI payload
                    // Update last received sequence number
                    g_session.sequence_rx_last = seq;

                    // Skip RTP fixed header (12 bytes)
                    const uint8_t *p = data + 12;
                    uint16_t remaining = (uint16_t)(len - 12);

                    // RTP-MIDI payload header (RFC 6295)
                    // Short header: 1 byte: B,J,Z,P,LEN(4)
                    // Long header: 2 bytes: B,J,Z,P,LEN(12)
                    if (remaining < 1U) {
                        netbuf_delete(buf);
                        continue;
                    }

                    uint8_t hdr0 = p[0];
                    uint8_t B = (uint8_t)((hdr0 >> 7) & 0x01);
                    uint8_t Z = (uint8_t)((hdr0 >> 5) & 0x01);

                    uint16_t cmd_len = 0;
                    uint16_t hdr_len = 1;

                    if (B == 0U) {
                        cmd_len = (uint16_t)(hdr0 & 0x0F);
                    } else {
                        // Need 2 bytes
                        if (remaining < 2U) {
                            netbuf_delete(buf);
                            continue;
                        }
                        cmd_len = (uint16_t)(((hdr0 & 0x0F) << 8) | p[1]);
                        hdr_len = 2;
                    }

                    if (remaining < hdr_len) {
                        netbuf_delete(buf);
                        continue;
                    }

                    p += hdr_len;
                    remaining = (uint16_t)(remaining - hdr_len);

                    // Some senders may include a Journal section (J bit) after the command section.
                    // This firmware does not implement Journal parsing yet; we only decode the command section.
                    if (cmd_len > remaining) {
                        // Malformed or truncated packet
                        netbuf_delete(buf);
                        continue;
                    }

                    const uint8_t *cmd = p;
                    const uint8_t *cmd_end = p + cmd_len;

                    // Decode MIDI command section
                    // If Z==0: first MIDI command starts immediately with status byte (no delta-time).
                    // If Z==1: first command is preceded by a delta-time.
                    const uint8_t *ptr = cmd;
                    uint8_t first_cmd = 1U;

                    while (ptr < cmd_end) {
                        // Delta-time handling (very small subset):
                        // - For Z==0 and first command: no delta-time.
                        // - Otherwise: consume 1 byte (or multiple for VLQ if MSB=1) conservatively.
                        if (!(first_cmd && (Z == 0U))) {
                            // Consume delta-time (VLQ)
                            do {
                                if (ptr >= cmd_end) {
                                    break;
                                }
                                uint8_t dt = *ptr++;
                                if ((dt & 0x80U) == 0U) {
                                    break;
                                }
                            } while (ptr < cmd_end);

                            if (ptr >= cmd_end) {
                                break;
                            }
                        }

                        first_cmd = 0U;

                        // Status byte
                        if (ptr >= cmd_end) {
                            break;
                        }
                        uint8_t status = *ptr++;
                        if ((status & 0x80U) == 0U) {
                            // Running status not supported in this minimal parser
                            break;
                        }

                        uint8_t msg_type = (uint8_t)(status & 0xF0U);
                        uint8_t data1 = 0U, data2 = 0U;

                        // NOTE: System messages (0xF*) are not handled here.
                        if (msg_type == MIDI_PROGRAM_CHANGE || msg_type == 0xD0U) {
                            // 1 data byte
                            if (ptr < cmd_end) {
                                data1 = *ptr++;
                            } else {
                                break;
                            }
                            g_rx_callback(status, data1, 0U);
                        } else {
                            // 2 data bytes
                            if (ptr < cmd_end) {
                                data1 = *ptr++;
                            } else {
                                break;
                            }
                            if (ptr < cmd_end) {
                                data2 = *ptr++;
                            } else {
                                break;
                            }
                            g_rx_callback(status, data1, data2);
                        }
                    }
                }
            }
            netbuf_delete(buf);
        }
    }
}
