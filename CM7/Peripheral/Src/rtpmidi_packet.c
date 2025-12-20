/**
 ******************************************************************************
 * @file           : rtpmidi_packet.c
 * @brief          : RTP-MIDI Packet Encoding Implementation
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
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define RTP_VERSION             2
#define RTP_PAYLOAD_TYPE_MIDI   0x61 // 97 - Standard RTP-MIDI payload type (RFC 6295)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static rtpmidi_status_t rtpmidi_send_packet(uint8_t *midi_data, uint16_t midi_len);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Send MIDI Control Change (7-bit)
 */
rtpmidi_status_t rtpmidi_send_cc(uint8_t channel, uint8_t cc, uint8_t value)
{
    if (!rtpmidi_is_connected()) {
        printf("RTP-MIDI Warning: Send CC failed - Not connected\n");
        return RTPMIDI_NOT_CONNECTED;
    }

    // Validate parameters
    if (channel > 15 || cc > 127 || value > 127) {
        return RTPMIDI_ERROR;
    }

    // Build MIDI message (3 bytes)
    uint8_t midi_msg[5];
    uint8_t *p = midi_msg;

    // Delta time (0 = immediate)
    *p++ = 0x00;

    // MIDI Control Change message
    *p++ = MIDI_CONTROL_CHANGE | (channel & 0x0F);
    *p++ = cc & 0x7F;
    *p++ = value & 0x7F;

    return rtpmidi_send_packet(midi_msg, p - midi_msg);
}

/**
 * @brief Send MIDI Control Change (14-bit high resolution)
 */
rtpmidi_status_t rtpmidi_send_cc14(uint8_t channel, uint8_t cc_msb, uint16_t value14)
{
    if (!rtpmidi_is_connected()) {
        printf("RTP-MIDI Warning: Send CC14 failed - Not connected\n");
        return RTPMIDI_NOT_CONNECTED;
    }

    // Validate parameters
    if (channel > 15 || cc_msb > 31 || value14 > 16383) {
        return RTPMIDI_ERROR;
    }

    // Build MIDI message (2 CC messages: MSB + LSB)
    uint8_t midi_msg[10];
    uint8_t *p = midi_msg;

    // First CC: MSB
    *p++ = 0x00;  // Delta time
    *p++ = MIDI_CONTROL_CHANGE | (channel & 0x0F);
    *p++ = cc_msb & 0x7F;
    *p++ = (value14 >> 7) & 0x7F;  // MSB (bits 13-7)

    // Second CC: LSB (CC number = MSB + 32)
    *p++ = 0x00;  // Delta time
    *p++ = MIDI_CONTROL_CHANGE | (channel & 0x0F);
    *p++ = (cc_msb + 32) & 0x7F;
    *p++ = value14 & 0x7F;  // LSB (bits 6-0)

    return rtpmidi_send_packet(midi_msg, p - midi_msg);
}

/**
 * @brief Send MIDI Note On/Off
 */
rtpmidi_status_t rtpmidi_send_note(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (!rtpmidi_is_connected()) {
        printf("RTP-MIDI Warning: Send Note failed - Not connected\n");
        return RTPMIDI_NOT_CONNECTED;
    }

    // Validate parameters
    if (channel > 15 || note > 127 || velocity > 127) {
        return RTPMIDI_ERROR;
    }

    // Build MIDI message
    uint8_t midi_msg[5];
    uint8_t *p = midi_msg;

    // Delta time
    *p++ = 0x00;

    // MIDI Note On/Off message
    if (velocity > 0) {
        *p++ = MIDI_NOTE_ON | (channel & 0x0F);
    } else {
        *p++ = MIDI_NOTE_OFF | (channel & 0x0F);
    }
    *p++ = note & 0x7F;
    *p++ = velocity & 0x7F;

    return rtpmidi_send_packet(midi_msg, p - midi_msg);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Send RTP-MIDI packet
 */
static rtpmidi_status_t rtpmidi_send_packet(uint8_t *midi_data, uint16_t midi_len)
{
    rtpmidi_session_t *session = rtpmidi_get_session();

    if (!session->conn_data) {
        printf("RTP-MIDI Error: No data connection\n");
        return RTPMIDI_ERROR;
    }

    // Calculate total packet size
    // RTP header (12 bytes) + RTP-MIDI payload header (1 byte for short header) + MIDI data (without delta time)
    uint16_t packet_size = 12 + 1 + (midi_len - 1);

    // Build packet in local buffer
    uint8_t packet[128];  // Should be enough for most MIDI messages
    if (packet_size > sizeof(packet)) {
        return RTPMIDI_ERROR;
    }

    uint8_t *p = packet;

    // --- RTP Header (12 bytes) ---
    // Byte 0: V=2, P=0 (no padding), X=0, CC=0
    *p++ = (RTP_VERSION << 6);  // P=0 like Mac

    // Byte 1: M=0 (marker, like Mac), PT=97 (0x61 = MIDI)
    *p++ = RTP_PAYLOAD_TYPE_MIDI;

    // Bytes 2-3: Sequence number
    uint16_t seq = htons(session->sequence_tx++);
    memcpy(p, &seq, 2);
    p += 2;

    // Bytes 4-7: Timestamp (10kHz clock)
    // Use current time converted to 100us units, adjusted with clock offset
    // Note: We use the lower 32 bits of the 64-bit timestamp
    uint64_t now_us = (uint64_t)HAL_GetTick() * 10;
    uint32_t adjusted_ts = (uint32_t)(now_us + session->clock_offset);
    session->timestamp = adjusted_ts;

    uint32_t timestamp = htonl(session->timestamp);
    memcpy(p, &timestamp, 4);
    p += 4;

    // Bytes 8-11: SSRC
    uint32_t ssrc = htonl(session->ssrc);
    memcpy(p, &ssrc, 4);
    p += 4;

    // --- RTP-MIDI Payload Header ---
    // Use SHORT HEADER format like Mac (B=0, J=0, Z=0, P=0)
    // For B=0: LEN is 4 bits only (bits 3-0 of first byte)
    uint8_t actual_midi_len = midi_len - 1; // Remove delta time byte
    *p++ = 0x00 | (actual_midi_len & 0x0F); // B=0, J=0, Z=0, P=0, LEN[3:0]

    // --- MIDI Data (Command Section) ---
    // Copy MIDI commands directly WITHOUT delta time (like Mac)
    // midi_data starts with 0x00 (delta time), skip it
    memcpy(p, midi_data + 1, actual_midi_len);
    p += actual_midi_len;

    // NO Journal Section (like Mac)
    // NO Padding (like Mac - P=0)

    // Send packet using netconn API (thread-safe)
    struct netbuf *buf = netbuf_new();
    if (!buf) {
        printf("RTP-MIDI Error: Failed to allocate netbuf\n");
        return RTPMIDI_BUFFER_FULL;
    }

    void *data = netbuf_alloc(buf, packet_size);
    if (!data) {
        netbuf_delete(buf);
        printf("RTP-MIDI Error: Failed to allocate buffer data\n");
        return RTPMIDI_BUFFER_FULL;
    }

    memcpy(data, packet, packet_size);

    err_t err = netconn_sendto(session->conn_data, buf, &session->remote_ip, session->remote_port_data);
    netbuf_delete(buf);

    if (err != ERR_OK) {
        printf("RTP-MIDI Error: netconn_sendto failed with error %d\n", err);
        return RTPMIDI_ERROR;
    }

    printf("RTP-MIDI: Sent packet seq=%d len=%d\n", seq, packet_size);
    return RTPMIDI_OK;
}
