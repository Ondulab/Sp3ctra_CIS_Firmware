/**
 ******************************************************************************
 * @file           : rtpmidi.h
 * @brief          : RTP-MIDI Protocol Implementation Header
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

#ifndef RTPMIDI_H
#define RTPMIDI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "lwip/api.h"
#include "lwip/ip_addr.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief RTP-MIDI operation modes
 */
typedef enum {
    RTPMIDI_MODE_SERVER = 0,  // AppleMIDI server (passive, waits for INVITE from macOS)
    RTPMIDI_MODE_CLIENT = 1   // RTP-MIDI client (active, initiates connection to PC/Linux)
} rtpmidi_mode_t;

/**
 * @brief RTP-MIDI session states
 */
typedef enum {
    RTPMIDI_STATE_IDLE = 0,
    RTPMIDI_STATE_INVITED,
    RTPMIDI_STATE_CONTROL_CONNECTED, // Control port handshake done
    RTPMIDI_STATE_CONNECTED,         // Data port handshake done
    RTPMIDI_STATE_SYNCHRONIZED       // Clock sync done
} rtpmidi_state_t;

/**
 * @brief RTP-MIDI status codes
 */
typedef enum {
    RTPMIDI_OK = 0,
    RTPMIDI_ERROR,
    RTPMIDI_NOT_CONNECTED,
    RTPMIDI_BUFFER_FULL,
    RTPMIDI_TIMEOUT
} rtpmidi_status_t;

/**
 * @brief MIDI message types
 */
typedef enum {
    MIDI_NOTE_OFF = 0x80,
    MIDI_NOTE_ON = 0x90,
    MIDI_CONTROL_CHANGE = 0xB0,
    MIDI_PROGRAM_CHANGE = 0xC0,
    MIDI_PITCH_BEND = 0xE0
} midi_message_type_t;

/**
 * @brief RTP-MIDI session context
 */
typedef struct {
    rtpmidi_mode_t mode;        // Operation mode (SERVER or CLIENT)
    rtpmidi_state_t state;
    uint32_t ssrc;              // Our SSRC (unique identifier)
    uint32_t remote_ssrc;       // Remote SSRC
    uint32_t initiator_token;   // Session token

    uint16_t sequence_tx;       // TX sequence number
    uint16_t sequence_tx_last_sent; // Last RTP sequence actually sent in a data packet
    uint16_t sequence_rx_last;  // Last RX sequence
    uint32_t timestamp;         // RTP timestamp (10kHz clock)

    uint32_t last_feedback_tick; // Last receiver feedback time
    int64_t clock_offset;        // Clock offset (remote - local)

    struct netconn *conn_control;  // UDP port 5004
    struct netconn *conn_data;     // UDP port 5005
    ip_addr_t remote_ip;
    // Fixed AppleMIDI control/data ports of the remote participant (typically N / N+1).
    uint16_t remote_port_control;
    uint16_t remote_port_data;

    // Actual UDP source ports used by the peer when initiating AppleMIDI.
    // macOS may use ephemeral source ports for IN/OK and expects replies to those ports.
    uint16_t peer_port_control;
    uint16_t peer_port_data;

    uint32_t last_sync_tick;    // Last clock sync time
    uint32_t last_invite_tick;  // Last invitation time
    uint8_t connection_attempts;
    uint8_t ck_sync_initiated;  // Flag: CK sync initiation sent (CLIENT mode)

    uint8_t device_name[64];
} rtpmidi_session_t;

/**
 * @brief Callback for incoming MIDI messages
 * @param status MIDI status byte (message type + channel)
 * @param data1 First data byte
 * @param data2 Second data byte
 */
typedef void (*rtpmidi_rx_callback_t)(uint8_t status, uint8_t data1, uint8_t data2);

/* Exported constants --------------------------------------------------------*/
#define RTPMIDI_CONTROL_PORT        DEFAULT_RTPMIDI_CONTROL_PORT
#define RTPMIDI_DATA_PORT           (DEFAULT_RTPMIDI_CONTROL_PORT + 1)
#define RTPMIDI_SYNC_INTERVAL_MS    10000  // 10 seconds
#define RTPMIDI_FEEDBACK_INTERVAL_MS 1000  // 1 second
#define RTPMIDI_INVITE_INTERVAL_MS  1000   // 1 second (Apple spec)
#define RTPMIDI_MAX_ATTEMPTS        12     // Apple spec

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize RTP-MIDI subsystem
 * @param device_name Name of this MIDI device
 * @param remote_ip IP address of remote peer (used in CLIENT mode, can be NULL in SERVER mode)
 * @param mode Operation mode (RTPMIDI_MODE_SERVER for macOS, RTPMIDI_MODE_CLIENT for PC/Linux)
 * @return RTPMIDI_OK on success, error code otherwise
 */
rtpmidi_status_t rtpmidi_init(const char *device_name, ip_addr_t *remote_ip, rtpmidi_mode_t mode);

/**
 * @brief Start connection to remote peer
 * @return RTPMIDI_OK on success, error code otherwise
 */
rtpmidi_status_t rtpmidi_connect(void);

/**
 * @brief Disconnect from remote peer
 * @return RTPMIDI_OK on success, error code otherwise
 */
rtpmidi_status_t rtpmidi_disconnect(void);

/**
 * @brief Process RTP-MIDI (call periodically from task)
 * Handles session management, timeouts, clock sync
 * @return RTPMIDI_OK on success, error code otherwise
 */
rtpmidi_status_t rtpmidi_process(void);

/**
 * @brief Send MIDI Control Change (7-bit)
 * @param channel MIDI channel (0-15)
 * @param cc Control Change number (0-127)
 * @param value Control value (0-127)
 * @return RTPMIDI_OK on success, error code otherwise
 */
rtpmidi_status_t rtpmidi_send_cc(uint8_t channel, uint8_t cc, uint8_t value);

/**
 * @brief Send MIDI Control Change (14-bit high resolution)
 * @param channel MIDI channel (0-15)
 * @param cc_msb Control Change MSB number (0-31)
 * @param value14 14-bit value (0-16383)
 * @return RTPMIDI_OK on success, error code otherwise
 */
rtpmidi_status_t rtpmidi_send_cc14(uint8_t channel, uint8_t cc_msb, uint16_t value14);

/**
 * @brief Send MIDI Note On/Off
 * @param channel MIDI channel (0-15)
 * @param note Note number (0-127)
 * @param velocity Velocity (0-127, 0=Note Off)
 * @return RTPMIDI_OK on success, error code otherwise
 */
rtpmidi_status_t rtpmidi_send_note(uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * @brief Register callback for incoming MIDI messages
 * @param callback Function to call when MIDI message received
 */
void rtpmidi_register_rx_callback(rtpmidi_rx_callback_t callback);

/**
 * @brief Check if RTP-MIDI is connected
 * @return 1 if connected, 0 otherwise
 */
uint8_t rtpmidi_is_connected(void);

/**
 * @brief Get current session state
 * @return Current state
 */
rtpmidi_state_t rtpmidi_get_state(void);

/**
 * @brief Get pointer to session (for internal use by packet module)
 * @return Pointer to session structure
 */
rtpmidi_session_t* rtpmidi_get_session(void);

#ifdef __cplusplus
}
#endif

#endif // RTPMIDI_H
