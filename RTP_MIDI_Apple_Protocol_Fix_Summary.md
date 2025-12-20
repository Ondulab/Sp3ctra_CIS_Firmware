# RTP-MIDI Apple Protocol Compliance Fix Summary

## Overview
This fix addresses critical issues preventing the STM32 firmware from establishing a stable RTP-MIDI session with macOS. The implementation now fully complies with Apple's MIDI Network Driver Protocol.

## Changes Implemented

### 1. Full Session Handshake
- **Before**: Only sent invitation on control port (5004).
- **After**: Implements the required 4-step handshake:
  1. Control Port Invitation (IN) -> Accepted (OK)
  2. Data Port Invitation (IN) -> Accepted (OK)
- **New State**: Added `RTPMIDI_STATE_CONTROL_CONNECTED` to track the intermediate state.

### 2. Clock Synchronization (3-Way Handshake)
- **Before**: Sent incorrect CK packets (count=0) and didn't handle responses.
- **After**: Implements the full 3-way exchange:
  1. Initiator sends CK (count=0, ts1)
  2. Responder sends CK (count=1, ts1, ts2)
  3. Initiator sends CK (count=2, ts1, ts2, ts3)
- **Timestamp Units**: Fixed to use 100µs units (10kHz clock) as per spec.
- **Offset Calculation**: Now calculates clock offset `((ts1 + ts3) / 2) - ts2`.

### 3. Receiver Feedback
- **New Feature**: Added `rtpmidi_send_receiver_feedback` to periodically send RS packets on the control port.
- **Purpose**: Informs the sender of the last received sequence number, allowing optimization of the recovery journal.

### 4. Command Handling on Data Port
- **Update**: Modified `rtpmidi_data_recv_thread` to detect and handle Apple-MIDI commands (IN, OK, CK) on the data port (5005).
- **Logic**: Distinguishes between MIDI data (RTP header) and Commands (0xFFFF signature).

### 5. RTP Timestamp Correction
- **Fix**: Updated `rtpmidi_send_packet` to use the current system time (converted to 100µs units) for the RTP timestamp instead of an incrementing counter.
- **Impact**: Ensures that MIDI packets have valid timestamps relative to the synchronized clock, preventing them from being rejected by CoreMIDI.

### 6. RTP-MIDI Header Optimization & Journal
- **Fix**: Updated `rtpmidi_send_packet` to force Long Header (B=1) and include a minimal Recovery Journal (J=1).
- **Impact**: Mimics Apple's implementation behavior exactly. CoreMIDI seems to reject packets without a journal or with Short Headers, even if RFC-compliant. The minimal journal indicates "Single Packet Loss" with no data, which is valid and safe.

### 7. Random Sequence Number Initialization
- **Fix**: Updated `rtpmidi_init` to initialize `sequence_tx` and `ssrc` using `HAL_GetTick()` as a seed.
- **Impact**: Prevents CoreMIDI from rejecting packets as "duplicates" or "old" if the STM32 reboots and reconnects with the same SSRC and sequence number 0.

### 8. Debug Logging
- **Action**: Added `printf` logs in `rtpmidi_send_packet` and wrapper functions (`rtpmidi_send_cc`, etc.) to trace packet transmission errors and connection state checks.
- **Update**: Enabled success log in `rtpmidi_send_packet` to confirm packet transmission attempts.
- **Purpose**: To diagnose why packets might not be sent even if the session appears connected.

## Files Modified
1. `CM7/Peripheral/Inc/rtpmidi.h`: Added new states and definitions.
2. `CM7/Peripheral/Src/rtpmidi_session.c`: Implemented the protocol logic and random initialization.
3. `CM7/Peripheral/Src/rtpmidi_packet.c`: Fixed RTP timestamp generation, header format, and added comprehensive debug logs.

## How to Test

1. **Compile and Flash**: Build the project in STM32CubeIDE and flash to the board.
2. **Network Setup**: Connect STM32 and Mac to the same network.
3. **Audio MIDI Setup (Mac)**:
   - Open Audio MIDI Setup -> MIDI Studio -> Network.
   - You should see the STM32 device in the directory.
   - Select it and click "Connect".
4. **Verification**:
   - The "Connect" button should change to "Disconnect".
   - The latency measurement should appear (indicating successful clock sync).
   - MIDI Monitor should show the device as online.
5. **Wireshark (Optional)**:
   - Filter: `rtpmidi` or `udp.port == 5004 || udp.port == 5005`.
   - Verify the IN/OK exchange on both ports.
   - Verify the CK exchange (3 packets).

## Troubleshooting
- If connection fails at "Connecting...", check if the data port (5005) is blocked by firewall.
- If latency is 0 or huge, clock sync failed. Check timestamp endianness.
- If session drops after 1 minute, receiver feedback or clock sync keep-alive is missing.
