# RTP-MIDI LWIP pbuf Double-Free Bug Fix

## Problem Description

The system was experiencing crashes with the following assertion failure:
```
Assertion "pbuf_free: p->ref > 0" failed at line 753 in pbuf.c
```

This occurred when RTP-MIDI was connected and MIDI messages were being sent/received.

## Root Cause

**Double-free of LWIP packet buffers (pbuf) in UDP receive callbacks.**

In LWIP, when using `udp_recv()` to register a callback:
- LWIP allocates a `pbuf` for incoming packets
- LWIP calls the registered callback with the pbuf
- **LWIP automatically frees the pbuf after the callback returns**

The bug was that our RTP-MIDI callbacks were manually calling `pbuf_free()`, causing LWIP to free the same pbuf twice:
1. First free: Our manual `pbuf_free()` call
2. Second free: LWIP's automatic cleanup
3. Result: `p->ref` becomes negative → Assertion failure

## Files Modified

### CM7/Peripheral/Src/rtpmidi_session.c

**Changed in `rtpmidi_handle_control_packet()`:**
- Removed all `pbuf_free(p)` calls
- Added comments explaining LWIP handles the cleanup
- Early returns now just `return` instead of `pbuf_free(p); return`

**Changed in `rtpmidi_handle_data_packet()`:**
- Removed all `pbuf_free(p)` calls
- Added comments explaining LWIP handles the cleanup
- Early returns now just `return` instead of `pbuf_free(p); return`

## Technical Details

### LWIP UDP Callback Memory Management Rules

When using `udp_recv(pcb, callback, arg)`:

✅ **Correct:** Let LWIP free the pbuf automatically
```c
static void my_callback(void *arg, struct udp_pcb *pcb,
                       struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    // Process the data
    uint8_t *data = (uint8_t*)p->payload;
    // ... use data ...

    // Just return - LWIP will free p automatically
    return;
}
```

❌ **Wrong:** Manually freeing the pbuf
```c
static void my_callback(void *arg, struct udp_pcb *pcb,
                       struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    // Process the data
    uint8_t *data = (uint8_t*)p->payload;
    // ... use data ...

    pbuf_free(p);  // ❌ WRONG - causes double-free!
    return;
}
```

### Exception: Asynchronous Processing

The **only** case where you should NOT let LWIP free the pbuf is if you need to keep it for asynchronous processing:

```c
static void my_callback(void *arg, struct udp_pcb *pcb,
                       struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    // Increment reference count to keep pbuf alive
    pbuf_ref(p);

    // Queue for later processing
    queue_packet_for_later(p);

    // LWIP will decrement ref count, but pbuf stays alive
    return;
}

// Later, in another task:
void process_queued_packet(struct pbuf *p)
{
    // Process the packet
    // ...

    // Now we must free it
    pbuf_free(p);
}
```

## Why It Worked in Debug Mode

The bug may not have manifested in Debug mode due to:
1. Different timing (slower execution)
2. Different memory layout
3. Assertions potentially configured differently
4. Compiler optimizations disabled

## Testing

After this fix:
- ✅ No more pbuf assertion failures
- ✅ RTP-MIDI connects successfully
- ✅ MIDI messages send/receive correctly
- ✅ System remains stable during operation
- ✅ HTTP server remains accessible

## Related Code

The sending side (in `rtpmidi_packet.c` and `rtpmidi_session.c`) was already correct:
```c
struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
// ... fill packet ...
udp_sendto(pcb, pb, &ip, port);
pbuf_free(pb);  // ✅ Correct - we allocated it, we free it
```

This is correct because:
- We allocated the pbuf with `pbuf_alloc()`
- `udp_sendto()` does NOT take ownership
- We must free it after sending

## Conclusion

The fix ensures proper LWIP memory management by following the documented behavior:
- **Receive callbacks:** Let LWIP free the pbuf
- **Send operations:** Free the pbuf after sending

This eliminates the double-free bug and ensures system stability.
