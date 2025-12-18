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
#include "lwip/pbuf.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define RTP_VERSION             2
#define RTP_PAYLOAD_TYPE_MIDI   0x61

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

    if (!session->pcb_data) {
        return RTPMIDI_ERROR;
    }

    // Calculate total packet size
    // RTP header (12 bytes) + RTP-MIDI payload header (2 bytes) + MIDI data
    uint16_t packet_size = 12 + 2 + midi_len;

    // Allocate packet buffer
    struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, packet_size, PBUF_RAM);
    if (!pb) {
        return RTPMIDI_BUFFER_FULL;
    }

    uint8_t *packet = (uint8_t*)pb->payload;
    uint8_t *p = packet;

    // --- RTP Header (12 bytes) ---
    // Byte 0: V=2, P=0, X=0, CC=0
    *p++ = (RTP_VERSION << 6);

    // Byte 1: M=1 (marker), PT=97 (0x61 = MIDI)
    *p++ = 0x80 | RTP_PAYLOAD_TYPE_MIDI;

    // Bytes 2-3: Sequence number
    uint16_t seq = htons(session->sequence_tx++);
    memcpy(p, &seq, 2);
    p += 2;

    // Bytes 4-7: Timestamp (10kHz clock)
    uint32_t timestamp = htonl(session->timestamp);
    memcpy(p, &timestamp, 4);
    p += 4;
    session->timestamp += 1;  // Increment by 1 for each packet

    // Bytes 8-11: SSRC
    uint32_t ssrc = htonl(session->ssrc);
    memcpy(p, &ssrc, 4);
    p += 4;

    // --- RTP-MIDI Payload Header (2 bytes) ---
    // Byte 0: B=0, J=0, Z=0, P=0, LEN[12:8]=0
    *p++ = 0x00;

    // Byte 1: LEN[7:0] = MIDI data length
    *p++ = midi_len & 0xFF;

    // --- MIDI Data ---
    memcpy(p, midi_data, midi_len);

    // Send packet
    err_t err = udp_sendto(session->pcb_data, pb, &session->remote_ip, session->remote_port_data);
    pbuf_free(pb);

    return (err == ERR_OK) ? RTPMIDI_OK : RTPMIDI_ERROR;
}
