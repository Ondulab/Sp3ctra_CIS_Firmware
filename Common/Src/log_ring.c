/**
 ******************************************************************************
 * @file           : log_ring.c
 * @brief          : Journal circulaire en RAM retenue, voir log_ring.h.
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

#include "log_ring.h"

#include <string.h>

#include "stm32h7xx.h"

_Static_assert(sizeof(log_ring_hdr_t) == LOG_RING_HDR_SIZE, "log_ring_hdr_t doit faire 32 octets");
_Static_assert(LOG_RING_CM4_ADDR + LOG_RING_HDR_SIZE + LOG_RING_CM4_SIZE == LOG_REGION_END,
               "les anneaux doivent remplir exactement la zone reservee");
_Static_assert((LOG_RING_CM7_ADDR % 32u) == 0u && (LOG_RING_CM4_ADDR % 32u) == 0u,
               "en-tetes alignes sur une ligne de cache");
_Static_assert((LOG_RING_CM7_SIZE % 32u) == 0u && (LOG_RING_CM4_SIZE % 32u) == 0u,
               "donnees en nombre entier de lignes de cache");

#define LOG_RING_MAGIC 0x31474C53u /* "SLG1" */
#define LOG_RING_SALT  0xA5C3F00Du

#if defined(BOOTLOADER)
#define LOG_TAG      'B'
#define LOG_OWN_ADDR LOG_RING_CM7_ADDR
#define LOG_OWN_SIZE LOG_RING_CM7_SIZE
#elif defined(CORE_CM7)
#define LOG_TAG      '7'
#define LOG_OWN_ADDR LOG_RING_CM7_ADDR
#define LOG_OWN_SIZE LOG_RING_CM7_SIZE
#else
#define LOG_TAG      '4'
#define LOG_OWN_ADDR LOG_RING_CM4_ADDR
#define LOG_OWN_SIZE LOG_RING_CM4_SIZE
#endif

/* HAL_GetTick vaut 0 tant que HAL_Init n'a pas tourne : acceptable. */
extern uint32_t HAL_GetTick(void);
/* Defini par usart.c quand la trace serie existe ; reference faible pour que le
 * CM4, qui n'initialise pas d'UART, se contente de l'anneau. */
extern int __io_putchar(int ch) __attribute__((weak));

static log_ring_hdr_t *const own      = (log_ring_hdr_t *)LOG_OWN_ADDR;
static uint8_t *const         own_data = (uint8_t *)(LOG_OWN_ADDR + LOG_RING_HDR_SIZE);
static bool own_ready;
static bool line_start = true;

/* Maintenance de cache : le CM7 ecrit derriere son D-cache, or un reset ne
 * recopie pas les lignes sales en RAM. Chaque ecriture est donc nettoyee vers
 * la memoire ; la lecture d'un anneau ecrit par l'autre coeur est precedee
 * d'une invalidation. Le CM4 n'a pas de cache. */
#if defined(CORE_CM7)
static void cache_clean(const void *addr, uint32_t len)
{
    uintptr_t a = (uintptr_t)addr & ~(uintptr_t)31u;
    uint32_t  n = (uint32_t)(((uintptr_t)addr + len - a + 31u) & ~(uintptr_t)31u);
    SCB_CleanDCache_by_Addr((uint32_t *)a, (int32_t)n);
}
static void cache_invalidate(const void *addr, uint32_t len)
{
    uintptr_t a = (uintptr_t)addr & ~(uintptr_t)31u;
    uint32_t  n = (uint32_t)(((uintptr_t)addr + len - a + 31u) & ~(uintptr_t)31u);
    SCB_InvalidateDCache_by_Addr((uint32_t *)a, (int32_t)n);
}
#else
#define cache_clean(addr, len)      ((void)0)
#define cache_invalidate(addr, len) ((void)0)
#endif

static uint32_t seal(uint32_t magic, uint32_t size)
{
    return magic ^ size ^ LOG_RING_SALT;
}

static bool hdr_valid(const log_ring_hdr_t *h, uint32_t size)
{
    return (h->magic == LOG_RING_MAGIC) && (h->size == size) && (h->check == seal(h->magic, h->size));
}

static const log_ring_hdr_t *ring_of(log_src_t src, uint32_t *size)
{
    if (src == LOG_SRC_CM4)
    {
        *size = LOG_RING_CM4_SIZE;
        return (const log_ring_hdr_t *)LOG_RING_CM4_ADDR;
    }
    *size = LOG_RING_CM7_SIZE;
    return (const log_ring_hdr_t *)LOG_RING_CM7_ADDR;
}

void log_ring_init(void)
{
    if (hdr_valid(own, LOG_OWN_SIZE))
    {
        own->boots++;
    }
    else
    {
        own->magic = LOG_RING_MAGIC;
        own->size  = LOG_OWN_SIZE;
        own->head  = 0u;
        own->boots = 0u;
        memset(own->reserved, 0, sizeof(own->reserved));
        own->check = seal(LOG_RING_MAGIC, LOG_OWN_SIZE);
    }
    own->tag = LOG_TAG;
    cache_clean(own, LOG_RING_HDR_SIZE);

    line_start = true;
    own_ready  = true;
}

/* "[T ssss.mmm] " -- 13 caracteres, sans passer par printf (reentrance). */
static size_t put_prefix(char *out)
{
    uint32_t t  = HAL_GetTick();
    uint32_t s  = t / 1000u;
    uint32_t ms = t % 1000u;
    char digits[10];
    int  nd = 0;
    size_t n = 0;

    out[n++] = '[';
    out[n++] = LOG_TAG;
    out[n++] = ' ';

    do
    {
        digits[nd++] = (char)('0' + (s % 10u));
        s /= 10u;
    } while (s != 0u && nd < 10);
    for (int pad = nd; pad < 4; pad++)
    {
        out[n++] = ' ';
    }
    while (nd > 0)
    {
        out[n++] = digits[--nd];
    }
    out[n++] = '.';
    out[n++] = (char)('0' + (ms / 100u));
    out[n++] = (char)('0' + ((ms / 10u) % 10u));
    out[n++] = (char)('0' + (ms % 10u));
    out[n++] = ']';
    out[n++] = ' ';
    return n;
}

static void flush(const char *buf, size_t n)
{
    if (own_ready && n > 0u)
    {
        uint32_t primask = __get_PRIMASK();
        __disable_irq();

        uint32_t start = own->head;
        uint32_t size  = own->size;
        for (size_t i = 0; i < n; i++)
        {
            own_data[(start + i) % size] = (uint8_t)buf[i];
        }
        own->head = start + (uint32_t)n;

        __set_PRIMASK(primask);

        /* Nettoyage de cache borne a ce qui vient d'etre ecrit. */
        uint32_t s = start % size;
        if (n >= size)
        {
            cache_clean(own_data, size);
        }
        else if (s + n <= size)
        {
            cache_clean(own_data + s, (uint32_t)n);
        }
        else
        {
            cache_clean(own_data + s, size - s);
            cache_clean(own_data, (uint32_t)n - (size - s));
        }
        cache_clean(own, LOG_RING_HDR_SIZE);
    }

#if LOG_RING_UART_ECHO
    if (&__io_putchar != NULL)
    {
        for (size_t i = 0; i < n; i++)
        {
            (void)__io_putchar(buf[i]);
        }
    }
#endif
}

void log_ring_write(const char *data, size_t len)
{
    char   chunk[96];
    size_t n = 0;

    for (size_t i = 0; i < len; i++)
    {
        if (line_start)
        {
            n += put_prefix(chunk + n);
            line_start = false;
        }
        chunk[n++] = data[i];
        if (data[i] == '\n')
        {
            line_start = true;
        }
        /* Assez de marge pour un prefixe suivi d'un caractere. */
        if (n >= sizeof(chunk) - 16u || i + 1u == len)
        {
            flush(chunk, n);
            n = 0;
        }
    }
}

bool log_ring_valid(log_src_t src)
{
    uint32_t size;
    const log_ring_hdr_t *h = ring_of(src, &size);
    if ((const void *)h != (const void *)own)
    {
        cache_invalidate(h, LOG_RING_HDR_SIZE);
    }
    return hdr_valid(h, size);
}

uint32_t log_ring_head(log_src_t src)
{
    uint32_t size;
    const log_ring_hdr_t *h = ring_of(src, &size);
    if ((const void *)h != (const void *)own)
    {
        cache_invalidate(h, LOG_RING_HDR_SIZE);
    }
    return hdr_valid(h, size) ? h->head : 0u;
}

uint32_t log_ring_read(log_src_t src, uint32_t since, void *out, uint32_t max, uint32_t *next)
{
    uint32_t size;
    const log_ring_hdr_t *h = ring_of(src, &size);
    const uint8_t *d = (const uint8_t *)h + LOG_RING_HDR_SIZE;

    if ((const void *)h != (const void *)own)
    {
        cache_invalidate(h, LOG_RING_HDR_SIZE + size);
    }
    if (!hdr_valid(h, size))
    {
        if (next != NULL)
        {
            *next = 0u;
        }
        return 0u;
    }

    uint32_t head   = h->head;
    uint32_t oldest = (head > size) ? (head - size) : 0u;
    if (since < oldest)
    {
        since = oldest;
    }
    if (since > head)
    {
        since = head;
    }

    uint32_t n = head - since;
    if (n > max)
    {
        n = max;
    }

    uint32_t s = since % size;
    if (s + n <= size)
    {
        memcpy(out, d + s, n);
    }
    else
    {
        memcpy(out, d + s, size - s);
        memcpy((uint8_t *)out + (size - s), d, n - (size - s));
    }

    if (next != NULL)
    {
        *next = since + n;
    }
    return n;
}

/* Remplace la version faible de syscalls.c : tout printf passe par l'anneau. */
int _write(int file, char *ptr, int len)
{
    (void)file;
    if (ptr != NULL && len > 0)
    {
        log_ring_write(ptr, (size_t)len);
    }
    return len;
}
