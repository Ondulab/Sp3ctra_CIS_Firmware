# RTP-MIDI Payload Format Fix

## Problem

MIDI Monitor was not receiving/displaying MIDI messages even though UDP packets were being sent and received correctly on port 5005.

### Diagnosis from tcpdump

```
Port 5005 packet payload:
0x0020:  0000 0004 0527 6414 0004 00b0 157f
              ^timestamp  ^SSRC   ^payload
```

The payload was: `0004 00b0 157f`
- `0004`: RTP-MIDI header (length = 4)
- `00`: Delta time
- `b0`: Control Change channel 0
- `15`: CC#21
- `7f`: Value 127

## Root Cause

The RTP-MIDI payload header format was incorrect according to RFC 6295 (RTP Payload Format for MIDI).

**Previous implementation:**
```c
*p++ = 0x00;           // All flags set to 0
*p++ = midi_len & 0xFF; // Length
```

This format had the **B flag** (Big command section present) set to 0, which meant MIDI receivers like MIDI Monitor would not recognize the command section.

## Solution

Changed the payload header to set the **B flag** to 1, indicating that a command section is present:

```c
// Byte 0: B=1 (Big command section), J=0, Z=0, P=0, LEN[11:8]=0
*p++ = 0x80;  // B flag set to indicate command section present

// Byte 1: LEN[7:0] = MIDI data length
*p++ = midi_len & 0xFF;
```

### RFC 6295 Payload Header Format

```
 0                   1
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|B|J|Z|P|LEN    |  LEN          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

Where:
- **B** (bit 0): Big command section present (1 = present)
- **J** (bit 1): Journal section present (0 = not present)
- **Z** (bit 2): Delta time for first command is zero (0 = not zero)
- **P** (bit 3): Phantom flag (0 = not phantom)
- **LEN** (bits 4-15): Length of MIDI command section in bytes

## Files Modified

- `CM7/Peripheral/Src/rtpmidi_packet.c`: Updated `rtpmidi_send_packet()` function

## Expected Result

After this fix, MIDI Monitor should now correctly receive and display:
- Control Change messages (7-bit and 14-bit)
- Note On/Off messages
- All other MIDI messages sent via RTP-MIDI

## Testing

To verify the fix:
1. Rebuild the firmware
2. Flash to STM32H7
3. Connect to the device via RTP-MIDI
4. Open MIDI Monitor
5. Send MIDI messages from the device
6. Verify messages appear in MIDI Monitor

Expected tcpdump output on port 5005 should now show:
```
0x0020:  0000 0004 0527 6414 0004 80b0 157f
                                    ^^ B flag set
```

## References

- RFC 6295: RTP Payload Format for MIDI
- Apple RTP-MIDI Implementation Guide
