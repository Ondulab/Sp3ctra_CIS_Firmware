# RTP-MIDI Implementation Roadmap
## Sp3ctra CIS Firmware - IMU & Buttons via Ethernet

**Version:** 1.0
**Date:** 2025-01-10
**Target:** STM32H745IIK6 Dual-Core (CM7 + CM4)

---

## Table des matières

1. [Vue d'ensemble](#vue-densemble)
2. [Architecture système](#architecture-système)
3. [Modules à implémenter](#modules-à-implémenter)
4. [Étapes d'implémentation](#étapes-dimplémentation)
5. [Configuration réseau](#configuration-réseau)
6. [Tests et validation](#tests-et-validation)
7. [Checklist d'implémentation](#checklist-dimplémentation)

---

## Vue d'ensemble

### Objectif

Ajouter un mécanisme RTP-MIDI (RFC 6295) permettant de :
- **Envoyer** les données IMU (6-DOF) en MIDI 14-bit via Ethernet
- **Envoyer** les états des boutons en MIDI Note/CC
- **Recevoir** des commandes MIDI pour contrôler les LEDs
- **Apparaître** comme périphérique MIDI côté PC (via rtpMIDI/Apple MIDI)

### Protocole RTP-MIDI

RTP-MIDI encapsule les messages MIDI dans des paquets RTP transmis via UDP :
- **Port 5004** : Control channel (session management)
- **Port 5005** : Data channel (MIDI messages)

### Résolution MIDI

- **7-bit standard** : 0-127 (Note, Velocity, CC simple)
- **14-bit haute résolution** : 0-16383 (CC MSB+LSB, Pitch Bend)
  - Pour IMU : résolution 14-bit recommandée
  - Gyro ±250°/s → 0.03°/s par step
  - Acc ±2g → 0.00024g par step

---

## Architecture système

```
┌─────────────────────────────────────────────────────────────┐
│                      STM32H745 CM7                          │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐ │
│  │ IMU Task     │───→│ MIDI Manager │───→│ RTP-MIDI TX  │ │
│  │ (100Hz)      │    │  (Encoding)  │    │  (UDP 5005)  │ │
│  └──────────────┘    └──────────────┘    └──────────────┘ │
│         ↑                    ↕                     ↓        │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐ │
│  │ Button ISR   │───→│ Event Queue  │    │ RTP-MIDI RX  │ │
│  │ (GPIO IT)    │    │ (Lock-free)  │    │  (UDP 5005)  │ │
│  └──────────────┘    └──────────────┘    └──────────────┘ │
│                              ↓                     ↓        │
│                      ┌──────────────┐    ┌──────────────┐ │
│                      │ HSEM / Shared│←───│ Session Mgr  │ │
│                      │    Memory    │    │  (UDP 5004)  │ │
│                      └──────────────┘    └──────────────┘ │
└────────────────────────────┬────────────────────────────────┘
                             │ HSEM notification
┌────────────────────────────▼────────────────────────────────┐
│                      STM32H745 CM4                          │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐ │
│  │ LED Control  │←───│ MIDI Handler │←───│ Shared Mem   │ │
│  │   (PWM)      │    │  (Decoder)   │    │   Reader     │ │
│  └──────────────┘    └──────────────┘    └──────────────┘ │
│         ↓                                                   │
│  ┌──────────────┐                                          │
│  │ GUI Update   │                                          │
│  │  (optional)  │                                          │
│  └──────────────┘                                          │
└─────────────────────────────────────────────────────────────┘
                             ↕
                    ┌──────────────────┐
                    │   PC (Ethernet)   │
                    │ rtpMIDI / DAW     │
                    └──────────────────┘
```

### Flux de données

#### Sortant (STM32 → PC)
1. IMU échantillonné à 100Hz → MIDI Manager
2. Conversion float → 14-bit MIDI CC
3. Throttling (envoi si changement > seuil)
4. Encapsulation RTP-MIDI
5. Transmission UDP port 5005

#### Entrant (PC → STM32)
1. Réception UDP port 5005
2. Décodage RTP-MIDI → messages MIDI
3. Écriture dans shared memory (D3 SRAM)
4. Notification HSEM vers CM4
5. CM4 lit commandes et pilote LEDs

---

## Modules à implémenter

### Module 1: RTP-MIDI Core (`CM7/Peripheral/`)

**Fichiers:**
- `rtpmidi.h` - API publique
- `rtpmidi.c` - Implémentation core
- `rtpmidi_session.c` - Gestion de session (IN/OK/BY/CK)
- `rtpmidi_packet.c` - Encodage/décodage paquets

**Responsabilités:**
- Session management (invitation, acceptation, synchronisation)
- Encodage MIDI → RTP-MIDI
- Décodage RTP-MIDI → MIDI
- Timestamps RTP et synchronisation clock

**Dépendances:**
- LWIP (UDP)
- FreeRTOS (tasks, semaphores)

### Module 2: MIDI Manager (`CM7/Application/`)

**Fichiers:**
- `midi_manager.h` - API publique
- `midi_manager.c` - Orchestration
- `midi_imu.c` - Mapping IMU → MIDI
- `midi_buttons.c` - Mapping Boutons → MIDI

**Responsabilités:**
- Queue d'événements MIDI
- Throttling IMU (éviter saturation réseau)
- Conversion physique (float) → MIDI (14-bit)
- Dispatch événements entrants vers CM4

**Dépendances:**
- RTP-MIDI Core
- ICM42688 driver
- GPIO (boutons)

### Module 3: Inter-Core Communication (`Common/`)

**Fichiers:**
- `midi_ipc.h` - Structures partagées
- `midi_ipc.c` - Helpers HSEM

**Responsabilités:**
- Structure de données partagée (D3 SRAM)
- Lock-free circular buffer pour events MIDI
- Notification HSEM CM7→CM4
- Cache coherency (DCache clean/invalidate)

**Dépendances:**
- HAL HSEM
- Linker script (section D3 SRAM)

### Module 4: LED Controller (`CM4/Peripheral/`)

**Fichiers à modifier:**
- `leds.c` - Ajouter mode MIDI
- `leds.h` - API étendue

**Responsabilités:**
- Mapping MIDI intensity (0-127) → PWM (0-255)
- Smooth transitions (optionnel)
- Polling IPC buffer

**Dépendances:**
- MIDI IPC
- TIM/PWM existant

---

## Étapes d'implémentation

### Phase 1: Fondations RTP-MIDI Core (Semaine 1)

#### 1.1 Structures de données

**Fichier:** `CM7/Peripheral/Inc/rtpmidi.h`

```c
#ifndef RTPMIDI_H
#define RTPMIDI_H

#include <stdint.h>
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

// Session states
typedef enum {
    RTPMIDI_STATE_IDLE = 0,
    RTPMIDI_STATE_INVITED,
    RTPMIDI_STATE_CONNECTED,
    RTPMIDI_STATE_SYNCHRONIZED
} rtpmidi_state_t;

// Status codes
typedef enum {
    RTPMIDI_OK = 0,
    RTPMIDI_ERROR,
    RTPMIDI_NOT_CONNECTED,
    RTPMIDI_BUFFER_FULL
} rtpmidi_status_t;

// Session context (singleton)
typedef struct {
    rtpmidi_state_t state;
    uint32_t ssrc;              // Our SSRC (unique)
    uint32_t remote_ssrc;
    uint32_t initiator_token;

    uint16_t sequence_tx;       // TX sequence number
    uint16_t sequence_rx_last;  // Last RX sequence
    uint32_t timestamp;         // RTP timestamp (10kHz)

    struct udp_pcb *pcb_control;  // UDP port 5004
    struct udp_pcb *pcb_data;     // UDP port 5005
    ip_addr_t remote_ip;
    uint16_t remote_port;

    uint64_t clock_offset;
    uint32_t last_sync_tick;

    uint8_t device_name[64];    // "Sp3ctra_CIS"
} rtpmidi_session_t;

// Public API
rtpmidi_status_t rtpmidi_init(const char *device_name);
rtpmidi_status_t rtpmidi_start_session(ip_addr_t *remote_ip);
rtpmidi_status_t rtpmidi_send_cc14(uint8_t channel, uint8_t cc_msb, uint16_t value14);
rtpmidi_status_t rtpmidi_send_note(uint8_t channel, uint8_t note, uint8_t velocity);
rtpmidi_status_t rtpmidi_process(void);  // Call from task

// Callback for incoming MIDI
typedef void (*rtpmidi_rx_callback_t)(uint8_t status, uint8_t data1, uint8_t data2);
void rtpmidi_register_rx_callback(rtpmidi_rx_callback_t callback);

#endif // RTPMIDI_H
```

#### 1.2 Session Management

**Fichier:** `CM7/Peripheral/Src/rtpmidi_session.c`

```c
#include "rtpmidi.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

static rtpmidi_session_t g_session = {0};

// Control packet handlers
static void rtpmidi_send_invitation(void);
static void rtpmidi_send_ok(uint32_t initiator_token);
static void rtpmidi_handle_control_packet(void *arg, struct udp_pcb *pcb,
                                           struct pbuf *p, const ip_addr_t *addr, u16_t port);

rtpmidi_status_t rtpmidi_init(const char *device_name)
{
    memset(&g_session, 0, sizeof(g_session));

    // Generate unique SSRC (based on MAC address + random)
    // TODO: Use HAL_GetUIDw0/1/2 for unique ID
    g_session.ssrc = 0x12345678;

    strncpy((char*)g_session.device_name, device_name, sizeof(g_session.device_name)-1);

    // Create UDP PCBs
    g_session.pcb_control = udp_new();
    g_session.pcb_data = udp_new();

    if (!g_session.pcb_control || !g_session.pcb_data) {
        printf("RTP-MIDI: Failed to create UDP PCBs\n");
        return RTPMIDI_ERROR;
    }

    // Bind control port (5004)
    if (udp_bind(g_session.pcb_control, IP_ADDR_ANY, 5004) != ERR_OK) {
        printf("RTP-MIDI: Failed to bind control port\n");
        return RTPMIDI_ERROR;
    }

    // Bind data port (5005)
    if (udp_bind(g_session.pcb_data, IP_ADDR_ANY, 5005) != ERR_OK) {
        printf("RTP-MIDI: Failed to bind data port\n");
        return RTPMIDI_ERROR;
    }

    // Register callbacks
    udp_recv(g_session.pcb_control, rtpmidi_handle_control_packet, NULL);
    udp_recv(g_session.pcb_data, rtpmidi_handle_data_packet, NULL);

    g_session.state = RTPMIDI_STATE_IDLE;
    printf("RTP-MIDI: Initialized on ports 5004/5005\n");

    return RTPMIDI_OK;
}

rtpmidi_status_t rtpmidi_start_session(ip_addr_t *remote_ip)
{
    if (g_session.state != RTPMIDI_STATE_IDLE) {
        return RTPMIDI_ERROR;
    }

    ip_addr_copy(g_session.remote_ip, *remote_ip);
    g_session.remote_port = 5004;

    rtpmidi_send_invitation();
    g_session.state = RTPMIDI_STATE_INVITED;

    printf("RTP-MIDI: Invitation sent to %s\n", ipaddr_ntoa(remote_ip));
    return RTPMIDI_OK;
}

static void rtpmidi_send_invitation(void)
{
    uint8_t packet[128];
    uint8_t *p = packet;

    // Signature
    *p++ = 0xFF; *p++ = 0xFF;
    // Command 'IN'
    *p++ = 'I'; *p++ = 'N';
    // Protocol version
    *(uint32_t*)p = htonl(0x00000002); p += 4;
    // Initiator token (random)
    g_session.initiator_token = HAL_GetTick();  // Simple random
    *(uint32_t*)p = htonl(g_session.initiator_token); p += 4;
    // SSRC
    *(uint32_t*)p = htonl(g_session.ssrc); p += 4;
    // Name
    strcpy((char*)p, (char*)g_session.device_name);
    p += strlen((char*)g_session.device_name) + 1;

    // Send via LWIP
    struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, p - packet, PBUF_RAM);
    if (pb) {
        memcpy(pb->payload, packet, p - packet);
        udp_sendto(g_session.pcb_control, pb, &g_session.remote_ip, g_session.remote_port);
        pbuf_free(pb);
    }
}

static void rtpmidi_handle_control_packet(void *arg, struct udp_pcb *pcb,
                                           struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    if (p->len < 8) {
        pbuf_free(p);
        return;
    }

    uint8_t *data = (uint8_t*)p->payload;
    uint16_t signature = (data[0] << 8) | data[1];

    if (signature != 0xFFFF) {
        pbuf_free(p);
        return;
    }

    uint16_t command = (data[2] << 8) | data[3];

    switch (command) {
        case 0x494E:  // 'IN' - Invitation
            // TODO: Handle invitation from remote
            break;

        case 0x4F4B:  // 'OK' - Accepted
            if (g_session.state == RTPMIDI_STATE_INVITED) {
                g_session.remote_ssrc = ntohl(*(uint32_t*)(data + 12));
                g_session.state = RTPMIDI_STATE_CONNECTED;
                printf("RTP-MIDI: Session accepted, SSRC=0x%08X\n", g_session.remote_ssrc);
            }
            break;

        case 0x434B:  // 'CK' - Clock sync
            // TODO: Handle clock sync
            break;

        case 0x4259:  // 'BY' - Goodbye
            g_session.state = RTPMIDI_STATE_IDLE;
            printf("RTP-MIDI: Session closed\n");
            break;
    }

    pbuf_free(p);
}
```

#### 1.3 Packet Encoding

**Fichier:** `CM7/Peripheral/Src/rtpmidi_packet.c`

```c
#include "rtpmidi.h"
#include "lwip/pbuf.h"
#include <string.h>

extern rtpmidi_session_t g_session;  // From rtpmidi_session.c

rtpmidi_status_t rtpmidi_send_cc14(uint8_t channel, uint8_t cc_msb, uint16_t value14)
{
    if (g_session.state != RTPMIDI_STATE_CONNECTED) {
        return RTPMIDI_NOT_CONNECTED;
    }

    uint8_t packet[32];
    uint8_t *p = packet;

    // RTP Header (12 bytes)
    *p++ = 0x80;                              // V=2, P=0, X=0, CC=0
    *p++ = 0xE1;                              // M=1, PT=97 (MIDI)
    *(uint16_t*)p = htons(g_session.sequence_tx++); p += 2;
    *(uint32_t*)p = htonl(g_session.timestamp);     p += 4;
    *(uint32_t*)p = htonl(g_session.ssrc);          p += 4;

    // RTP-MIDI Payload Header (2 bytes)
    *p++ = 0x00;                              // B=0, J=0, Z=0, P=0, LEN[12:8]=0
    *p++ = 0x06;                              // LEN[7:0]=6 (6 bytes MIDI: 2 CC messages)

    // MIDI Command 1: CC MSB
    *p++ = 0x00;                              // Delta time = 0
    *p++ = 0xB0 | (channel & 0x0F);           // Control Change
    *p++ = cc_msb & 0x7F;                     // CC number (MSB)
    *p++ = (value14 >> 7) & 0x7F;             // Value MSB

    // MIDI Command 2: CC LSB (CC+32)
    *p++ = 0x00;                              // Delta time = 0
    *p++ = 0xB0 | (channel & 0x0F);           // Control Change
    *p++ = (cc_msb + 32) & 0x7F;              // CC number (LSB)
    *p++ = value14 & 0x7F;                    // Value LSB

    // Update timestamp (10kHz clock)
    g_session.timestamp += 1;  // Increment by 1 for each packet

    // Send via LWIP
    struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, p - packet, PBUF_RAM);
    if (pb) {
        memcpy(pb->payload, packet, p - packet);
        err_t err = udp_sendto(g_session.pcb_data, pb, &g_session.remote_ip, 5005);
        pbuf_free(pb);
        return (err == ERR_OK) ? RTPMIDI_OK : RTPMIDI_ERROR;
    }

    return RTPMIDI_ERROR;
}

rtpmidi_status_t rtpmidi_send_note(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (g_session.state != RTPMIDI_STATE_CONNECTED) {
        return RTPMIDI_NOT_CONNECTED;
    }

    uint8_t packet[32];
    uint8_t *p = packet;

    // RTP Header
    *p++ = 0x80;
    *p++ = 0xE1;
    *(uint16_t*)p = htons(g_session.sequence_tx++); p += 2;
    *(uint32_t*)p = htonl(g_session.timestamp);     p += 4;
    *(uint32_t*)p = htonl(g_session.ssrc);          p += 4;

    // RTP-MIDI Payload Header
    *p++ = 0x00;
    *p++ = 0x03;  // 3 bytes MIDI

    // MIDI Command: Note On/Off
    *p++ = 0x00;  // Delta time = 0
    *p++ = (velocity > 0 ? 0x90 : 0x80) | (channel & 0x0F);
    *p++ = note & 0x7F;
    *p++ = velocity & 0x7F;

    g_session.timestamp += 1;

    // Send
    struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, p - packet, PBUF_RAM);
    if (pb) {
        memcpy(pb->payload, packet, p - packet);
        err_t err = udp_sendto(g_session.pcb_data, pb, &g_session.remote_ip, 5005);
        pbuf_free(pb);
        return (err == ERR_OK) ? RTPMIDI_OK : RTPMIDI_ERROR;
    }

    return RTPMIDI_ERROR;
}
```

---

### Phase 2: MIDI Manager & IMU Mapping (Semaine 1-2)

#### 2.1 IMU to MIDI Mapping

**Fichier:** `CM7/Application/Inc/midi_imu.h`

```c
#ifndef MIDI_IMU_H
#define MIDI_IMU_H

#include <stdint.h>

// IMU to MIDI mapping config
typedef struct {
    uint8_t cc_msb;         // CC number (MSB), LSB = MSB+32
    float   range_min;      // Physical min value
    float   range_max;      // Physical max value
    float   deadzone;       // Center deadzone (0 = disabled)
    uint8_t smooth_factor;  // 0-255 (0=no smooth, 255=max smooth)
} imu_midi_map_t;

// IMU axes enum
typedef enum {
    IMU_GYRO_X = 0,
    IMU_GYRO_Y,
    IMU_GYRO_Z,
    IMU_ACC_X,
    IMU_ACC_Y,
    IMU_ACC_Z,
    IMU_AXIS_COUNT
} imu_axis_t;

// Default mapping: CC 16-21 for 6DOF
extern const imu_midi_map_t imu_default_mapping[IMU_AXIS_COUNT];

// Initialize IMU MIDI subsystem
void midi_imu_init(void);

// Process IMU data and send MIDI (call at IMU rate, e.g., 100Hz)
void midi_imu_process(void);

// Enable/disable specific axis
void midi_imu_set_axis_enabled(imu_axis_t axis, uint8_t enabled);

// Update mapping at runtime (optional)
void midi_imu_set_mapping(imu_axis_t axis, const imu_midi_map_t *map);

#endif // MIDI_IMU_H
```

**Fichier:** `CM7/Application/Src/midi_imu.c`

```c
#include "midi_imu.h"
#include "rtpmidi.h"
#include "icm42688.h"
#include <math.h>
#include <string.h>

// Default mapping (CC 16-21, 14-bit)
const imu_midi_map_t imu_default_mapping[IMU_AXIS_COUNT] = {
    {16, -250.0f, 250.0f, 2.0f, 50},   // Gyro X (±250°/s, 2° deadzone)
    {17, -250.0f, 250.0f, 2.0f, 50},   // Gyro Y
    {18, -250.0f, 250.0f, 2.0f, 50},   // Gyro Z
    {19, -2.0f, 2.0f, 0.05f, 80},      // Acc X (±2g, 0.05g deadzone)
    {20, -2.0f, 2.0f, 0.05f, 80},      // Acc Y
    {21, -2.0f, 2.0f, 0.05f, 80},      // Acc Z
};

static imu_midi_map_t current_mapping[IMU_AXIS_COUNT];
static uint8_t axis_enabled[IMU_AXIS_COUNT] = {1,1,1,1,1,1};
static uint16_t last_values[IMU_AXIS_COUNT] = {0};

void midi_imu_init(void)
{
    memcpy(current_mapping, imu_default_mapping, sizeof(current_mapping));
    memset(last_values, 0, sizeof(last_values));
}

void midi_imu_process(void)
{
    float raw_values[IMU_AXIS_COUNT] = {
        icm42688_gyrX(), icm42688_gyrY(), icm42688_gyrZ(),
        icm42688_accX(), icm42688_accY(), icm42688_accZ()
    };

    for (int i = 0; i < IMU_AXIS_COUNT; i++) {
        if (!axis_enabled[i]) continue;

        const imu_midi_map_t *map = &current_mapping[i];
        float value = raw_values[i];

        // Apply deadzone
        if (map->deadzone > 0.0f && fabsf(value) < map->deadzone) {
            value = 0.0f;
        }

        // Normalize to 0-16383 (14-bit)
        float normalized = (value - map->range_min) / (map->range_max - map->range_min);
        normalized = fmaxf(0.0f, fminf(1.0f, normalized));  // Clamp
        uint16_t value14 = (uint16_t)(normalized * 16383.0f);

        // Smoothing (exponential moving average)
        if (map->smooth_factor > 0) {
            value14 = (last_values[i] * map->smooth_factor + value14 * (255 - map->smooth_factor)) / 255;
        }

        // Send only if changed significantly (reduce MIDI traffic)
        uint16_t delta = (value14 > last_values[i]) ?
                         (value14 - last_values[i]) : (last_values[i] - value14);

        if (delta >= 16) {  // ~0.1% threshold
            rtpmidi_send_cc14(0, map->cc_msb, value14);
            last_values[i] = value14;
        }
    }
}

void midi_imu_set_axis_enabled(imu_axis_t axis, uint8_t enabled)
{
    if (axis < IMU_AXIS_COUNT) {
        axis_enabled[axis] = enabled;
    }
}

void midi_imu_set_mapping(imu_axis_t axis, const imu_midi_map_t *map)
{
    if (axis < IMU_AXIS_COUNT && map) {
        current_mapping[axis] = *map;
    }
}
```

#### 2.2 Buttons to MIDI

**Fichier:** `CM7/Application/Inc/midi_buttons.h`

```c
#ifndef MIDI_BUTTONS_H
#define MIDI_BUTTONS_H

#include <stdint.h>
#include "config.h"  // NUMBER_OF_BUTTONS

// Button MIDI mode
typedef enum {
    BUTTON_MODE_NOTE = 0,   // Note On/Off
    BUTTON_MODE_CC,         // Control Change (0/127)
    BUTTON_MODE_TOGGLE      // CC toggle (0→127→0)
} button_midi_mode_t;

// Button mapping
typedef struct {
    button_midi_mode_t mode;
    uint8_t channel;
    uint8_t note_or_cc;     // Note number or CC number
    uint8_t velocity;       // For Note mode
} button_midi_map_t;

// Initialize
void midi_buttons_init(void);

// Process button state change (called from ISR or task)
void midi_buttons_on_change(uint8_t button_id, uint8_t pressed);

// Set mapping
void midi_buttons_set_mapping(uint8_t button_id, const button_midi_map_t *map);

// Incoming MIDI to button LED (registered as callback)
void midi_buttons_handle_rx(uint8_t status, uint8_t data1, uint8_t data2);

#endif // MIDI_BUTTONS_H
```

**Fichier:** `CM7/Application/Src/midi_buttons.c`

```c
#include "midi_buttons.h"
#include "rtpmidi.h"
#include "midi_ipc.h"
#include <string.h>

static button_midi_map_t button_maps[NUMBER_OF_BUTTONS];
static uint8_t button_toggle_state[NUMBER_OF_BUTTONS] = {0};

void midi_buttons_init(void)
{
    // Default: Note mode, C3-C3+N, channel 0
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        button_maps[i].mode = BUTTON_MODE_NOTE;
        button_maps[i].channel = 0;
        button_maps[i].note_or_cc = 60 + i;  // Middle C + offset
        button_maps[i].velocity = 100;
    }

    // Register RX callback
    rtpmidi_register_rx_callback(midi_buttons_handle_rx);
}

void midi_buttons_on_change(uint8_t button_id, uint8_t pressed)
{
    if (button_id >= NUMBER_OF_BUTTONS) return;

    const button_midi_map_t *map = &button_maps[button_id];

    switch (map->mode) {
        case BUTTON_MODE_NOTE:
            // Note On (velocity) or Note Off (velocity 0)
            rtpmidi_send_note(map->channel, map->note_or_cc,
                             pressed ? map->velocity : 0);
            break;

        case BUTTON_MODE_CC:
            // CC 0 or 127
            rtpmidi_send_cc14(map->channel, map->note_or_cc,
                             pressed ? 16383 : 0);
            break;

        case BUTTON_MODE_TOGGLE:
            // Toggle on press only
            if (pressed) {
                button_toggle_state[button_id] = !button_toggle_state[button_id];
                rtpmidi_send_cc14(map->channel, map->note_or_cc,
                                 button_toggle_state[button_id] ? 16383 : 0);
            }
            break;
    }
}

void midi_buttons_set_mapping(uint8_t button_id, const button_midi_map_t *map)
{
    if (button_id < NUMBER_OF_BUTTONS && map) {
        button_maps[button_id] = *map;
    }
}

void midi_buttons_handle_rx(uint8_t status, uint8_t data1, uint8_t data2)
{
    uint8_t msg_type = status & 0xF0;
    uint8_t channel = status & 0x0F;

    // Map MIDI messages to LED control via IPC
    if (msg_type == 0x90 && data2 > 0) {  // Note On
        // Note → LED intensity
        if (data1 < NUMBER_OF_BUTTONS) {
            midi_ipc_set_led_intensity(data1, data2);  // 0-127 → brightness
        }
    }
    else if (msg_type == 0x80 || (msg_type == 0x90 && data2 == 0)) {  // Note Off
        if (data1 < NUMBER_OF_BUTTONS) {
            midi_ipc_set_led_intensity(data1, 0);
        }
    }
    else if (msg_type == 0xB0) {  // Control Change
        // CC → LED control
        if (data1 < NUMBER_OF_BUTTONS) {
            midi_ipc_set_led_intensity(data1, data2);
        }
    }
}
```

---

### Phase 3: Inter-Core Communication (Semaine 2)

#### 3.1 Shared Memory Structure

**Fichier:** `Common/Inc/midi_ipc.h`

```c
#ifndef MIDI_IPC_H
#define MIDI_IPC_H

#include <stdint.h>
#include "config.h"

#define MIDI_IPC_MAGIC  0x4D494449  // "MIDI"
#define MIDI_IPC_BUFFER_SIZE 32

// LED command (CM7 → CM4)
typedef struct {
    uint8_t led_id;
    uint8_t intensity;  // 0-127 MIDI range
    uint16_t reserved;
} __attribute__((packed)) midi_led_cmd_t;

// Shared memory (in D3 SRAM, e.g., 0x38000000)
typedef struct {
    uint32_t magic;
    volatile uint32_t cm7_write_idx;
    volatile uint32_t cm4_read_idx;

    midi_led_cmd_t led_commands[MIDI_IPC_BUFFER_SIZE];  // Circular buffer

} __attribute__((aligned(32))) midi_ipc_t;

// Placement in linker script (D3 SRAM)
extern midi_ipc_t __midi_ipc_data;

// CM7 functions
void midi_ipc_init_cm7(void);
void midi_ipc_set_led_intensity(uint8_t led_id, uint8_t intensity);

// CM4 functions
void midi_ipc_init_cm4(void);
uint8_t midi_ipc_get_led_command(midi_led_cmd_t *cmd);

#endif // MIDI_IPC_H
```

#### 3.2 Lock-free Implementation

**Fichier:** `Common/Src/midi_ipc.c`

```c
#include "midi_ipc.h"
#include "stm32h7xx_hal.h"
#include <string.h>

// D3 SRAM placement (via linker script)
midi_ipc_t __midi_ipc_data __attribute__((section(".midi_ipc_section")));

void midi_ipc_init_cm7(void)
{
    memset(&__midi_ipc_data, 0, sizeof(__midi_ipc_data));
    __midi_ipc_data.magic = MIDI_IPC_MAGIC;

    // Ensure write visibility
    SCB_CleanDCache_by_Addr((uint32_t*)&__midi_ipc_data, sizeof(__midi_ipc_data));
}

void midi_ipc_set_led_intensity(uint8_t led_id, uint8_t intensity)
{
    uint32_t next_idx = (__midi_ipc_data.cm7_write_idx + 1) % MIDI_IPC_BUFFER_SIZE;

    // Check if buffer full
    if (next_idx == __midi_ipc_data.cm4_read_idx) {
        return;  // Drop command
    }

    // Write command
    __midi_ipc_data.led_commands[__midi_ipc_data.cm7_write_idx].led_id = led_id;
    __midi_ipc_data.led_commands[__midi_ipc_data.cm7_write_idx].intensity = intensity;

    // Clean cache before updating index
    SCB_CleanDCache_by_Addr((uint32_t*)&__midi_ipc_data.led_commands[__midi_ipc_data.cm7_write_idx],
                             sizeof(midi_led_cmd_t));

    // Update write index
    __midi_ipc_data.cm7_write_idx = next_idx;
    SCB_CleanDCache_by_Addr((uint32_t*)&__midi_ipc_data.cm7_write_idx, sizeof(uint32_t));

    // Notify CM4 via HSEM
    HAL_HSEM_FastTake(HSEM_ID_0);
    HAL_HSEM_Release(HSEM_ID_0, 0);
}

// CM4 side
void midi_ipc_init_cm4(void)
{
    // Invalidate cache
    SCB_InvalidateDCache_by_Addr((uint32_t*)&__midi_ipc_data, sizeof(__midi_ipc_data));

    // Setup HSEM notification
    HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));
}

uint8_t midi_ipc_get_led_command(midi_led_cmd_t *cmd)
{
    // Invalidate cache
    SCB_InvalidateDCache_by_Addr((uint32_t*)&__midi_ipc_data.cm4_read_idx, sizeof(uint32_t));
    SCB_InvalidateDCache_by_Addr((uint32_t*)&__midi_ipc_data.cm7_write_idx, sizeof(uint32_t));

    if (__midi_ipc_data.cm4_read_idx == __midi_ipc_data.cm7_write_idx) {
        return 0;  // Buffer empty
    }

    // Invalidate command data
    SCB_InvalidateDCache_by_Addr((uint32_t*)&__midi_ipc_data.led_commands[__midi_ipc_data.cm4_read_idx],
                                  sizeof(midi_led_cmd_t));

    // Read command
    *cmd = __midi_ipc_data.led_commands[__midi_ipc_data.cm4_read_idx];

    // Update read index
    __midi_ipc_data.cm4_read_idx = (__midi_ipc_data.cm4_read_idx + 1) % MIDI_IPC_BUFFER_SIZE;
    SCB_CleanDCache_by_Addr((uint32_t*)&__midi_ipc_data.cm4_read_idx, sizeof(uint32_t));

    return 1;  // Command available
}
```

#### 3.3 Linker Script Modification

**Fichier:** `CM7/STM32H745IIKX_FLASH.ld` et `CM4/STM32H745IIKX_FLASH.ld`

Ajouter dans la section MEMORY:
```ld
D3_SRAM (xrw) : ORIGIN = 0x38000000, LENGTH = 64K
```

Ajouter dans la section SECTIONS:
```ld
.midi_ipc_section (NOLOAD) :
{
    . = ALIGN(32);
    *(.midi_ipc_section)
    . = ALIGN(32);
} >D3_SRAM
```

---

### Phase 4: LED Controller CM4 (Semaine 2)

#### 4.1 Extension du module LED

**Fichier:** `CM4/Peripheral/Inc/leds.h` (ajouts)

```c
// MIDI LED mode
void leds_set_midi_mode(uint8_t enabled);
void leds_set_from_midi(uint8_t led_id, uint8_t midi_intensity);
```

**Fichier:** `CM4/Peripheral/Src/leds.c` (ajouts)

```c
static uint8_t midi_mode_enabled = 0;

void leds_set_midi_mode(uint8_t enabled)
{
    midi_mode_enabled = enabled;
}

void leds_set_from_midi(uint8_t led_id, uint8_t midi_intensity)
{
    if (!midi_mode_enabled || led_id >= NUMBER_OF_LEDS) return;

    // Convert MIDI 0-127 → LED PWM 0-255 (or 0-1000 if using TIM)
    uint16_t pwm_value = (uint16_t)(midi_intensity * 2);  // 127 → 254

    // Update LED PWM (via existing API)
    leds_set_brightness(led_id, pwm_value);
}
```

#### 4.2 Task CM4 pour IPC

**Fichier:** `CM4/Core/Src/main.c` (dans USER CODE sections)

```c
/* USER CODE BEGIN Header_StartMidiLedTask */
/**
  * @brief  Function implementing the midiLed thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartMidiLedTask */
void StartMidiLedTask(void const * argument)
{
    /* USER CODE BEGIN StartMidiLedTask */
    midi_ipc_init_cm4();
    leds_set_midi_mode(1);

    for (;;)
    {
        midi_led_cmd_t cmd;

        // Poll IPC buffer
        while (midi_ipc_get_led_command(&cmd)) {
            leds_set_from_midi(cmd.led_id, cmd.intensity);
        }

        osDelay(10);  // 100Hz poll rate
    }
    /* USER CODE END StartMidiLedTask */
}

/* USER CODE BEGIN 4 */
// In main(), create task:
// osThreadDef(midiLed, StartMidiLedTask, osPriorityNormal, 0, 128);
// osThreadCreate(osThread(midiLed), NULL);
/* USER CODE END 4 */
```

---

### Phase 5: Integration & FreeRTOS Tasks (Semaine 3)

#### 5.1 MIDI Manager Task (CM7)

**Fichier:** `CM7/Core/Src/freertos.c`

```c
/* USER CODE BEGIN Header_StartMidiTask */
/**
  * @brief  Function implementing the midiTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartMidiTask */
void StartMidiTask(void const * argument)
{
    /* USER CODE BEGIN StartMidiTask */
    // Wait for network to be ready
    osSemaphoreWait(udpReadySemaphoreHandle, osWaitForever);

    // Initialize RTP-MIDI
    rtpmidi_init("Sp3ctra_CIS");

    // Initialize MIDI subsystems
    midi_imu_init();
    midi_buttons_init();
    midi_ipc_init_cm7();

    // Auto-connect to configured destination
    ip_addr_t dest_ip;
    IP4_ADDR(&dest_ip, shared_config.network_dest_ip[0],
                       shared_config.network_dest_ip[1],
                       shared_config.network_dest_ip[2],
                       shared_config.network_dest_ip[3]);
    rtpmidi_start_session(&dest_ip);

    uint32_t last_imu_tick = 0;

    for (;;)
    {
        // Process RTP-MIDI (handle incoming packets, timeouts, etc.)
        rtpmidi_process();

        // Send IMU data at 100Hz
        uint32_t now = HAL_GetTick();
        if (now - last_imu_tick >= 10) {  // 10ms = 100Hz
            midi_imu_process();
            last_imu_tick = now;
        }

        osDelay(1);  // 1ms task period
    }
    /* USER CODE END StartMidiTask */
}

/* USER CODE BEGIN 4 */
// In main(), create task:
// osThreadDef(midiTask, StartMidiTask, osPriorityNormal, 0, 512);
// osThreadCreate(osThread(midiTask), NULL);
/* USER CODE END 4 */
```

#### 5.2 Button ISR Integration

**Fichier:** `CM7/Core/Src/gpio.c` ou dans le handler d'interruption

```c
/* USER CODE BEGIN 2 */
#include "midi_buttons.h"
/* USER CODE END 2 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* USER CODE BEGIN HAL_GPIO_EXTI_Callback */
    // Determine button ID from GPIO_Pin
    uint8_t button_id = 0;
    uint8_t pressed = 0;

    // Example mapping (adjust to your hardware)
    switch (GPIO_Pin) {
        case BUTTON_1_Pin:
            button_id = 0;
            pressed = HAL_GPIO_ReadPin(BUTTON_1_GPIO_Port, BUTTON_1_Pin) == GPIO_PIN_RESET;
            break;
        case BUTTON_2_Pin:
            button_id = 1;
            pressed = HAL_GPIO_ReadPin(BUTTON_2_GPIO_Port, BUTTON_2_Pin) == GPIO_PIN_RESET;
            break;
        // Add more buttons...
    }

    // Send MIDI event
    midi_buttons_on_change(button_id, pressed);

    /* USER CODE END HAL_GPIO_EXTI_Callback */
}
```

---

## Configuration réseau

### LWIP Configuration

**Fichier:** `CM7/LWIP/Target/lwipopts.h`

Vérifier/ajouter:
```c
#define LWIP_UDP                1
#define LWIP_NETCONN            1
#define LWIP_SOCKET             0  // Not needed for netconn API

// Memory pools (adjust if needed)
#define MEMP_NUM_UDP_PCB        8  // At least 2 for RTP-MIDI
#define MEMP_NUM_NETCONN        8
#define PBUF_POOL_SIZE          16

// UDP options
#define UDP_TTL                 255
```

### Network Configuration

Dans `Common/Inc/config.h`, ajouter:
```c
// RTP-MIDI Configuration
#define RTPMIDI_ENABLED         1
#define RTPMIDI_AUTO_CONNECT    1  // Auto-connect on network up
#define RTPMIDI_DEVICE_NAME     "Sp3ctra_CIS"
```

---

## Tests et validation

### Test 1: Session Establishment

**Objectif:** Vérifier que la session RTP-MIDI s'établit correctement

**Procédure:**
1. Installer rtpMIDI (Windows) ou utiliser Apple MIDI (macOS)
2. Configurer rtpMIDI pour écouter sur le réseau
3. Démarrer le STM32
4. Vérifier dans les logs:
   ```
   RTP-MIDI: Initialized on ports 5004/5005
   RTP-MIDI: Invitation sent to 192.168.0.100
   RTP-MIDI: Session accepted, SSRC=0x12345678
   ```
5. Vérifier dans rtpMIDI que "Sp3ctra_CIS" apparaît comme périphérique connecté

**Critères de succès:**
- ✅ Session établie sans erreur
- ✅ Périphérique visible dans rtpMIDI
- ✅ Pas de timeout ou déconnexion

### Test 2: IMU to MIDI

**Objectif:** Vérifier que les données IMU sont correctement converties en MIDI

**Procédure:**
1. Ouvrir un MIDI monitor (MIDI-OX, MIDI Monitor, etc.)
2. Connecter au périphérique "Sp3ctra_CIS"
3. Bouger le STM32 (rotation, accélération)
4. Observer les messages MIDI CC 14-bit sur les canaux 16-21

**Critères de succès:**
- ✅ Messages MIDI CC reçus à ~100Hz
- ✅ Valeurs 14-bit cohérentes (0-16383)
- ✅ Réactivité aux mouvements
- ✅ Deadzone fonctionnelle (pas de bruit au repos)

### Test 3: Buttons to MIDI

**Objectif:** Vérifier que les boutons envoient des messages MIDI

**Procédure:**
1. Appuyer sur chaque bouton
2. Observer dans MIDI monitor:
   - Note On (velocity 100) lors de l'appui
   - Note Off (velocity 0) lors du relâchement

**Critères de succès:**
- ✅ Note On/Off pour chaque bouton
- ✅ Pas de double-trigger
- ✅ Latence < 10ms

### Test 4: MIDI to LED

**Objectif:** Vérifier que les LEDs répondent aux commandes MIDI

**Procédure:**
1. Depuis un DAW ou MIDI controller, envoyer:
   - Note On (note 60-63, velocity 0-127)
   - Control Change (CC 0-3, value 0-127)
2. Observer les LEDs changer d'intensité

**Critères de succès:**
- ✅ LEDs répondent aux Note On/Off
- ✅ LEDs répondent aux CC
- ✅ Intensité proportionnelle à la velocity/value
- ✅ Latence < 20ms

### Test 5: Performance & Stability

**Objectif:** Vérifier la stabilité sur longue durée

**Procédure:**
1. Laisser tourner pendant 1 heure avec:
   - IMU actif (mouvements continus)
   - Boutons pressés aléatoirement
   - Commandes MIDI envoyées depuis PC
2. Monitorer:
   - CPU usage (CM7 et CM4)
   - Memory usage
   - Packet loss
   - Latency

**Critères de succès:**
- ✅ CPU CM7 < 30%
- ✅ CPU CM4 < 20%
- ✅ Pas de memory leak
- ✅ Packet loss < 0.1%
- ✅ Latency moyenne < 5ms

---

## Checklist d'implémentation

### Phase 1: RTP-MIDI Core ✓
- [ ] Créer `CM7/Peripheral/Inc/rtpmidi.h`
- [ ] Créer `CM7/Peripheral/Src/rtpmidi_session.c`
- [ ] Créer `CM7/Peripheral/Src/rtpmidi_packet.c`
- [ ] Implémenter session management (IN/OK/BY)
- [ ] Implémenter encodage CC 14-bit
- [ ] Implémenter encodage Note On/Off
- [ ] Tester avec rtpMIDI (session establishment)

### Phase 2: MIDI Manager ✓
- [ ] Créer `CM7/Application/Inc/midi_imu.h`
- [ ] Créer `CM7/Application/Src/midi_imu.c`
- [ ] Implémenter mapping IMU → MIDI 14-bit
- [ ] Implémenter throttling et smoothing
- [ ] Créer `CM7/Application/Inc/midi_buttons.h`
- [ ] Créer `CM7/Application/Src/midi_buttons.c`
- [ ] Implémenter mapping boutons → MIDI
- [ ] Tester IMU → MIDI avec MIDI monitor
- [ ] Tester boutons → MIDI avec MIDI monitor

### Phase 3: Inter-Core Communication ✓
- [ ] Créer `Common/Inc/midi_ipc.h`
- [ ] Créer `Common/Src/midi_ipc.c`
- [ ] Modifier linker scripts (CM7 et CM4)
- [ ] Implémenter circular buffer lock-free
- [ ] Implémenter cache coherency
- [ ] Tester IPC avec données de test

### Phase 4: LED Controller CM4 ✓
- [ ] Modifier `CM4/Peripheral/Inc/leds.h`
- [ ] Modifier `CM4/Peripheral/Src/leds.c`
- [ ] Créer task MIDI LED (CM4)
- [ ] Implémenter polling IPC
- [ ] Tester MIDI → LED avec commandes manuelles

### Phase 5: Integration ✓
- [ ] Créer task MIDI Manager (CM7)
- [ ] Intégrer ISR boutons
- [ ] Configurer LWIP (lwipopts.h)
- [ ] Ajouter configuration réseau
- [ ] Tests d'intégration complets
- [ ] Optimisation performance
- [ ] Documentation finale

### Phase 6: Tests & Validation ✓
- [ ] Test session establishment
- [ ] Test IMU → MIDI
- [ ] Test boutons → MIDI
- [ ] Test MIDI → LED
- [ ] Test performance & stabilité
- [ ] Test avec DAW (Ableton, Max/MSP, etc.)
- [ ] Validation finale

---

## Ressources et références

### Documentation
- **RFC 6295**: RTP Payload Format for MIDI
  https://datatracker.ietf.org/doc/html/rfc6295
- **MIDI Specification**: https://www.midi.org/specifications
- **rtpMIDI**: https://www.tobias-erichsen.de/software/rtpmidi.html
- **Apple MIDI**: Intégré dans macOS (Audio MIDI Setup)

### Outils de développement
- **MIDI Monitor** (macOS): https://www.snoize.com/MIDIMonitor/
- **MIDI-OX** (Windows): http://www.midiox.com/
- **Wireshark**: Pour analyser les paquets RTP-MIDI

### Estimation des ressources

**Mémoire Flash:**
- RTP-MIDI Core: ~15-20 KB
- MIDI Manager: ~10-15 KB
- IPC: ~2 KB
- **Total: ~30 KB**

**Mémoire RAM:**
- Session state: ~500 bytes
- IPC buffer: ~2 KB (D3 SRAM)
- Buffers LWIP: ~4 KB
- **Total: ~7 KB**

**CPU (estimation):**
- CM7: 5-10% @ 480MHz
- CM4: 2-5% @ 240MHz

**Bande passante réseau:**
- IMU 6-DOF @ 100Hz: ~1.2 KB/s
- Boutons (occasionnel): négligeable
- **Total: ~1.5 KB/s**

---

## Notes d'implémentation

### Priorités FreeRTOS recommandées
- MIDI Task (CM7): `osPriorityNormal`
- MIDI LED Task (CM4): `osPriorityNormal`
- LWIP Task: `osPriorityHigh` (existant)

### Considérations RT
- Pas de malloc/free dans les chemins critiques
- Buffers préalloués statiquement
- Lock-free IPC pour éviter les mutex
- Cache coherency stricte (DCache clean/invalidate)

### Debugging
- Activer printf via UART pour logs RTP-MIDI
- Utiliser Wireshark pour capturer paquets UDP
- MIDI Monitor pour vérifier messages MIDI
- Oscilloscope logique pour timing ISR boutons

---

**Fin du document**
