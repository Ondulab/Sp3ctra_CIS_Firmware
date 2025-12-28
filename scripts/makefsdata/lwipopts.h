#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* Minimal lwIP options for host-build of makefsdata */

#define LWIP_NOASSERT 0

#define LWIP_TCP 1
#define LWIP_UDP 1

#define LWIP_TCP_TIMESTAMPS 0
#define TCP_MSS 1460

/* required by some lwIP headers */
#define LWIP_PLATFORM_DIAG(x) do { (void)(x); } while(0)

#endif /* LWIPOPTS_H */
