/**
 ******************************************************************************
 * @file           : netboot_eth.c
 * @brief          : Glue HAL Ethernet + PHY LAN8742, en scrutation.
 *
 * Reprend les reglages de l'application (ethernetif.c : broches RMII AF11 sur
 * PA1/PA2/PA7, PC1/PC4/PC5, PG11/PG13/PG14) sans lwIP ni interruption : le
 * HAL v2 se pilote par HAL_ETH_ReadData() / HAL_ETH_Transmit() et reclame
 * seulement les trois callbacks d'allocation et de chainage ci-dessous.
 * Descripteurs et tampons vivent en D2 (section .netboot_eth du .ld).
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

#include "netboot_eth.h"

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "lan8742.h"

#define NB_ETH_BUF_SIZE 1536u
/* Un tampon de plus que de descripteurs : celui en cours de traitement, plus
 * une marge pour que le DMA ne manque jamais de place. */
#define NB_ETH_RX_POOL  (ETH_RX_DESC_CNT + 2u)

ETH_HandleTypeDef nb_heth;

static ETH_TxPacketConfigTypeDef nb_txcfg;
static lan8742_Object_t          nb_phy;
static bool                      nb_started;
static uint8_t                   nb_mac[6];

static ETH_DMADescTypeDef nb_rxdesc[ETH_RX_DESC_CNT] __attribute__((section(".netboot_eth"), aligned(32)));
static ETH_DMADescTypeDef nb_txdesc[ETH_TX_DESC_CNT] __attribute__((section(".netboot_eth"), aligned(32)));
static uint8_t nb_rxpool[NB_ETH_RX_POOL][NB_ETH_BUF_SIZE] __attribute__((section(".netboot_eth"), aligned(32)));
static uint8_t nb_txbuf[NB_ETH_BUF_SIZE] __attribute__((section(".netboot_eth"), aligned(32)));
static uint8_t nb_rxused[NB_ETH_RX_POOL];

/* Un paquet recu = un tampon (RxBuffLen couvre toute trame standard). Une
 * trame plus longue arrive en plusieurs morceaux : elle est jetee. */
typedef struct {
    uint8_t *buf;
    uint16_t len;
    bool     overflow;
} nb_rxpkt_t;

static nb_rxpkt_t nb_rxpkt;

/* ---- Pool de reception --------------------------------------------------- */

static uint8_t *rx_alloc(void)
{
    for (uint32_t i = 0; i < NB_ETH_RX_POOL; i++)
    {
        if (nb_rxused[i] == 0u)
        {
            nb_rxused[i] = 1u;
            return nb_rxpool[i];
        }
    }
    return NULL;
}

static void rx_free(const uint8_t *buf)
{
    for (uint32_t i = 0; i < NB_ETH_RX_POOL; i++)
    {
        if (nb_rxpool[i] == buf)
        {
            nb_rxused[i] = 0u;
            return;
        }
    }
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    *buff = rx_alloc();
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
    if (*pStart == NULL)
    {
        nb_rxpkt.buf      = buff;
        nb_rxpkt.len      = Length;
        nb_rxpkt.overflow = false;
        *pStart           = &nb_rxpkt;
        *pEnd             = &nb_rxpkt;
    }
    else
    {
        nb_rxpkt.overflow = true;
        rx_free(buff);
    }
}

void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
    (void)buff;
}

/* ---- PHY ----------------------------------------------------------------- */

static int32_t phy_io_init(void)
{
    HAL_ETH_SetMDIOClockRange(&nb_heth);
    return 0;
}

static int32_t phy_io_deinit(void)
{
    return 0;
}

static int32_t phy_io_read(uint32_t dev, uint32_t reg, uint32_t *val)
{
    return (HAL_ETH_ReadPHYRegister(&nb_heth, dev, reg, val) == HAL_OK) ? 0 : -1;
}

static int32_t phy_io_write(uint32_t dev, uint32_t reg, uint32_t val)
{
    return (HAL_ETH_WritePHYRegister(&nb_heth, dev, reg, val) == HAL_OK) ? 0 : -1;
}

static int32_t phy_io_tick(void)
{
    return (int32_t)HAL_GetTick();
}

static lan8742_IOCtx_t nb_phy_io = {phy_io_init, phy_io_deinit, phy_io_write, phy_io_read, phy_io_tick};

static void phy_reset_pulse(void)
{
    HAL_GPIO_WritePin(ETH_RST_GPIO_Port, ETH_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(ETH_RST_GPIO_Port, ETH_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
}

/* ---- MSP ----------------------------------------------------------------- */

void HAL_ETH_MspInit(ETH_HandleTypeDef *h)
{
    GPIO_InitTypeDef g = {0};

    if (h->Instance != ETH)
    {
        return;
    }

    __HAL_RCC_ETH1MAC_CLK_ENABLE();
    __HAL_RCC_ETH1TX_CLK_ENABLE();
    __HAL_RCC_ETH1RX_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF11_ETH;

    g.Pin = GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14; /* TX_EN, TXD0, TXD1 */
    HAL_GPIO_Init(GPIOG, &g);
    g.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;    /* MDC, RXD0, RXD1   */
    HAL_GPIO_Init(GPIOC, &g);
    g.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;    /* REF_CLK, MDIO, CRS_DV */
    HAL_GPIO_Init(GPIOA, &g);
}

/* ---- API ----------------------------------------------------------------- */

bool nb_eth_init(const uint8_t mac[6])
{
    GPIO_InitTypeDef g = {0};

    memcpy(nb_mac, mac, 6);
    memset(nb_rxused, 0, sizeof(nb_rxused));
    nb_started = false;

    /* Ligne de reset du PHY relachee, sans impulsion : si le lien etait etabli
     * avant le reset du MCU, il le reste. Le PHY n'est remis a zero que s'il
     * ne repond pas sur le MDIO. */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_WritePin(ETH_RST_GPIO_Port, ETH_RST_Pin, GPIO_PIN_SET);
    g.Pin   = ETH_RST_Pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ETH_RST_GPIO_Port, &g);
    HAL_Delay(20);

    memset(&nb_heth, 0, sizeof(nb_heth));
    nb_heth.Instance            = ETH;
    nb_heth.Init.MACAddr        = nb_mac;
    nb_heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
    nb_heth.Init.TxDesc         = nb_txdesc;
    nb_heth.Init.RxDesc         = nb_rxdesc;
    nb_heth.Init.RxBuffLen      = NB_ETH_BUF_SIZE;
    if (HAL_ETH_Init(&nb_heth) != HAL_OK)
    {
        printf("NETBOOT: HAL_ETH_Init failed (0x%08lX)\n", (unsigned long)nb_heth.ErrorCode);
        return false;
    }

    memset(&nb_txcfg, 0, sizeof(nb_txcfg));
    nb_txcfg.Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD;
    nb_txcfg.CRCPadCtrl = ETH_CRC_PAD_INSERT;

    memset(&nb_phy, 0, sizeof(nb_phy));
    LAN8742_RegisterBusIO(&nb_phy, &nb_phy_io);
    if (LAN8742_Init(&nb_phy) != LAN8742_STATUS_OK)
    {
        printf("NETBOOT: PHY silent on MDIO, pulsing its reset line\n");
        phy_reset_pulse();
        if (LAN8742_Init(&nb_phy) != LAN8742_STATUS_OK)
        {
            printf("NETBOOT: PHY not found\n");
            return false;
        }
    }
    return true;
}

int nb_eth_service(void)
{
    int32_t st = LAN8742_GetLinkState(&nb_phy);
    bool    up = (st >= LAN8742_STATUS_100MBITS_FULLDUPLEX) && (st <= LAN8742_STATUS_10MBITS_HALFDUPLEX);

    if (up && !nb_started)
    {
        ETH_MACConfigTypeDef mac;
        HAL_ETH_GetMACConfig(&nb_heth, &mac);
        mac.DuplexMode = (st == LAN8742_STATUS_100MBITS_FULLDUPLEX || st == LAN8742_STATUS_10MBITS_FULLDUPLEX)
                             ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;
        mac.Speed = (st == LAN8742_STATUS_100MBITS_FULLDUPLEX || st == LAN8742_STATUS_100MBITS_HALFDUPLEX)
                        ? ETH_SPEED_100M : ETH_SPEED_10M;
        HAL_ETH_SetMACConfig(&nb_heth, &mac);
        if (HAL_ETH_Start(&nb_heth) == HAL_OK)
        {
            nb_started = true;
        }
    }
    else if (!up && nb_started)
    {
        HAL_ETH_Stop(&nb_heth);
        nb_started = false;
    }
    return nb_started ? 1 : 0;
}

uint16_t nb_eth_poll(uint8_t *out, uint16_t max)
{
    void       *pkt = NULL;
    nb_rxpkt_t *p;
    uint16_t    n = 0u;

    if (!nb_started)
    {
        return 0u;
    }
    if (HAL_ETH_ReadData(&nb_heth, &pkt) != HAL_OK || pkt == NULL)
    {
        return 0u;
    }

    p = (nb_rxpkt_t *)pkt;
    if (!p->overflow && p->len >= 14u && p->len <= max)
    {
        memcpy(out, p->buf, p->len);
        n = p->len;
    }
    rx_free(p->buf);
    return n;
}

bool nb_eth_send(const uint8_t *frame, uint16_t len)
{
    ETH_BufferTypeDef b;
    HAL_StatusTypeDef st;

    if (!nb_started || len == 0u || len > NB_ETH_BUF_SIZE)
    {
        return false;
    }
    memcpy(nb_txbuf, frame, len);
    b.buffer = nb_txbuf;
    b.len    = len;
    b.next   = NULL;

    nb_txcfg.Length   = len;
    nb_txcfg.TxBuffer = &b;
    nb_txcfg.pData    = NULL;

    st = HAL_ETH_Transmit(&nb_heth, &nb_txcfg, 20);
    HAL_ETH_ReleaseTxPacket(&nb_heth);
    return st == HAL_OK;
}
