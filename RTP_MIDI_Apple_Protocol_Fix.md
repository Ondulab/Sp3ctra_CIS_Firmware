# RTP-MIDI Apple Protocol Compliance Fix

## Problems Identified

After analyzing Apple's MIDI Network Driver Protocol documentation, several critical issues were found:

### 1. Incomplete Session Handshake

**Apple Protocol Requirements:**
```
1. Initiator sends IN on control port
2. Responder sends OK/NO on control port
3. Initiator sends IN on MIDI data port  ← MISSING
4. Responder sends OK/NO on MIDI data port  ← MISSING
5. Initiator initiates clock synchronization
```

**Current Implementation:**
- Only sends invitation on control port
- Never completes the data port handshake
- This explains why macOS doesn't recognize the session

### 2. Clock Synchronization Protocol Violation

**Apple Protocol Requirements:**
```
Packet format:
- Signature (0xFFFF)
- Command (0x434B = "CK")
- SSRC (4 bytes)
- Count (1 byte): number of timestamps - 1
- Padding (3 bytes)
- Timestamp 1 (8 bytes, 64-bit)
- Timestamp 2 (8 bytes, 64-bit) - only if count >= 1
- Timestamp 3 (8 bytes, 64-bit) - only if count >= 2

Synchronization sequence:
1. Initiator: count=0, timestamp1=current_time
2. Responder: count=1, timestamp2=current_time, timestamp1=copied
3. Initiator: count=2, timestamp3=current_time, timestamp1+2=copied
```

**Current Implementation:**
- Always sends count=0 with 3 timestamps (incorrect)
- Never responds to received CK packets
- Doesn't implement the 3-way sync exchange
- Timestamps are in wrong units (should be 100µs, not 10kHz)

### 3. Missing Receiver Feedback

**Apple Protocol Requirements:**
- Receiver must periodically send feedback on control port
- Format: 0xFFFF + 0x5253 ("RS") + SSRC + sequence_number
- Allows sender to optimize recovery journal size

**Current Implementation:**
- Never sends receiver feedback
- Recovery journal always includes full state

### 4. State Machine Issues

**Required States:**
- IDLE: No connection
- INVITED: Waiting for control port OK
- CONTROL_CONNECTED: Control handshake done, waiting for data port handshake
- CONNECTED: Both handshakes done, waiting for sync
- SYNCHRONIZED: Fully operational

**Current Implementation:**
- Missing CONTROL_CONNECTED state
- Transitions directly from INVITED to CONNECTED
- Doesn't track data port handshake separately

## Implementation Plan

### Phase 1: Complete Session Handshake
1. Add CONTROL_CONNECTED state
2. Send invitation on data port after control port OK
3. Wait for data port OK before considering session connected
4. Handle both initiator and responder roles correctly

### Phase 2: Fix Clock Synchronization
1. Implement proper CK packet format with variable timestamp count
2. Implement 3-way sync exchange:
   - Send count=0 initially
   - Respond to count=0 with count=1
   - Respond to count=1 with count=2
3. Fix timestamp units (100µs intervals)
4. Calculate clock offset: `((ts3 + ts1) / 2) - ts2`
5. Send sync exchanges more frequently during startup

### Phase 3: Add Receiver Feedback
1. Track received packet sequence numbers
2. Periodically send RS packets on control port
3. Include highest received sequence number

### Phase 4: Improve State Management
1. Add proper state transitions
2. Handle timeout scenarios
3. Support both initiator and responder roles
4. Handle simultaneous connection attempts (both sides initiate)

## Technical Details

### Timestamp Format
- Unit: 100 microseconds (10 kHz)
- 64-bit unsigned integer
- Network byte order (big-endian)
- Relative to arbitrary point in past
- Lower 32 bits used in RTP packet headers

### Port Usage
- Control port: N (typically 5004)
- Data port: N+1 (typically 5005)
- Both use UDP
- Advertised via Bonjour: `_apple-midi._udp`

### Recovery Journal
Apple implements these chapters:
- P (Program Change)
- C (Control Change)
- W (Pitch Wheel)
- N (Note On/Off)
- T (Channel Aftertouch)
- A (Poly Aftertouch)
- Q (Sequencer state)
- F (MIDI Time Code)

Not implemented:
- M (MIDI Parameter System)
- E (Note Command Extras)
- D (Song Select, etc.)
- V (Active Sense)
- X (System Exclusive)

## Testing Strategy

1. **Wireshark Capture**: Compare packet format with macOS native implementation
2. **MIDI Monitor**: Verify session appears in macOS MIDI setup
3. **Timing Analysis**: Measure clock sync accuracy
4. **Stress Test**: Multiple rapid connect/disconnect cycles
5. **Bidirectional Test**: Test both initiator and responder roles

## References

- Apple MIDI Network Driver Protocol Documentation
- RFC 6295: RTP Payload Format for MIDI
- Current implementation: `CM7/Peripheral/Src/rtpmidi_session.c`
