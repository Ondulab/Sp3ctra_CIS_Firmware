/**
 ******************************************************************************
 * @file           : netboot.c
 * @brief          : Mode flasheur reseau du bootloader : entree, boucle,
 *                   portage du coeur (flash, journal, logs, identite).
 *
 * Tout tourne en nu, dans une seule boucle : lire une trame, y repondre, puis
 * la suivante. Les operations longues (effacement d'un secteur, ~1,7 s)
 * bloquent la boucle ; les trames s'accumulent dans l'anneau de reception et
 * l'hote attend l'accuse de reception avec un delai plus long.
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

#include "netboot.h"

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "fmc.h"
#include "gpio.h"

#include "boot_config.h"
#include "boot_mailbox.h"
#include "log_ring.h"
#include "netboot_core.h"
#include "netboot_eth.h"
#include "ota.h"
#include "ota_boot.h"
#include "stm32_flash.h"
#include "sys_identity.h"
#include "update_gui.h"

/* Sans hote dans ce delai, une entree par boite aux lettres ou par boutons
 * rend la main au demarrage normal ; une entree sur journal FAILED reste. */
#define NETBOOT_IDLE_TIMEOUT_MS 120000u
#define NETBOOT_LINK_POLL_MS    200u

static netboot_reason_t nb_reason;
static boot_mailbox_t   nb_mailbox;
static bool             nb_have_mailbox;
static nb_netcfg_t      nb_cfg;
static uint8_t          nb_frame[1536] __attribute__((aligned(4)));
static char             nb_ipstr[20];

static uint32_t running_sector_base(void);

/* ---- Entree -------------------------------------------------------------- */

bool netboot_entryRequested(netboot_reason_t *reason)
{
    GPIO_InitTypeDef g = {0};
    boot_mailbox_t   mb;

    *reason = NETBOOT_REASON_NONE;

    /* Demande de l'application : seulement apres un reset logiciel, pour
     * qu'une boite laissee par un plantage ne piege pas la machine. */
    if (boot_mailbox_take_request(&mb))
    {
        if (mb.request == BOOT_REQ_NETBOOT && (otaBoot_resetCause() & RCC_RSR_SFT1RSTF) != 0u)
        {
            nb_mailbox      = mb;
            nb_have_mailbox = true;
            *reason         = NETBOOT_REASON_MAILBOX;
            return true;
        }
        printf("NETBOOT: stale mailbox request ignored\n");
    }

    /* Deux boutons exterieurs (PE13 + PE15) maintenus a la mise sous tension.
     * Le bouton du milieu, contre l'ecran, n'y participe pas. */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    g.Pin  = SW_1_Pin | SW_2_Pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &g);
    HAL_Delay(5);
    if (HAL_GPIO_ReadPin(SW_1_GPIO_Port, SW_1_Pin) == GPIO_PIN_RESET &&
        HAL_GPIO_ReadPin(SW_2_GPIO_Port, SW_2_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(200); /* anti-rebond : ils doivent etre encore tenus */
        if (HAL_GPIO_ReadPin(SW_1_GPIO_Port, SW_1_Pin) == GPIO_PIN_RESET &&
            HAL_GPIO_ReadPin(SW_2_GPIO_Port, SW_2_Pin) == GPIO_PIN_RESET)
        {
            *reason = NETBOOT_REASON_BUTTONS;
            return true;
        }
    }
    return false;
}

/* ---- Affichage ----------------------------------------------------------- */

static void center32(char out[33], const char *s)
{
    size_t n = strlen(s);
    size_t pad;

    if (n > 32u)
    {
        n = 32u;
    }
    pad = (32u - n) / 2u;
    memset(out, ' ', 32);
    memcpy(out + pad, s, n);
    out[32] = '\0';
}

static void nb_display(const char *line1, const char *line2)
{
    char a[33];
    char b[33];
    center32(a, line1);
    center32(b, line2);
    gui_displayMessage(a, b);
}

static const char *reason_str(netboot_reason_t r)
{
    switch (r)
    {
    case NETBOOT_REASON_MAILBOX: return "requested by the application";
    case NETBOOT_REASON_BUTTONS: return "buttons held at power-on";
    case NETBOOT_REASON_FAILED:  return "OTA journal in FAILED state";
    default:                     return "unknown";
    }
}

/* ---- Boucle -------------------------------------------------------------- */

void netboot_run(netboot_reason_t reason)
{
    static const uint8_t def_ip[4]   = {192, 168, 100, 1};
    static const uint8_t def_mask[4] = {255, 255, 255, 0};
    uint32_t t_enter;
    uint32_t t_link     = 0u;
    int      link       = -1;
    bool     ever_bound = false;

    nb_reason = reason;
    printf("NETBOOT: network flash mode (%s)\n", reason_str(reason));
    otaBoot_refreshWatchdog();

    /* Le DMA Ethernet lit et ecrit la RAM directement : sans cache de donnees
     * il n'y a aucune coherence a entretenir, et la vitesse est de toute facon
     * bornee par la programmation de la flash. Le cache d'instructions reste. */
    SCB_DisableDCache();

    /* Ce que l'etape tardive initialise avant d'afficher : broches, FMC, OLED. */
    MX_GPIO_Init();
    MX_FMC_Init();
    gui_init();

    memset(&nb_cfg, 0, sizeof(nb_cfg));
    sys_identity_mac(nb_cfg.mac);
    memcpy(nb_cfg.ip, def_ip, 4);
    memcpy(nb_cfg.netmask, def_mask, 4);
    if (nb_have_mailbox && (nb_mailbox.ip[0] | nb_mailbox.ip[1] | nb_mailbox.ip[2] | nb_mailbox.ip[3]) != 0u)
    {
        memcpy(nb_cfg.ip, nb_mailbox.ip, 4);
        if ((nb_mailbox.netmask[0] | nb_mailbox.netmask[1] | nb_mailbox.netmask[2] | nb_mailbox.netmask[3]) != 0u)
        {
            memcpy(nb_cfg.netmask, nb_mailbox.netmask, 4);
        }
    }
    snprintf(nb_ipstr, sizeof(nb_ipstr), "%u.%u.%u.%u",
             nb_cfg.ip[0], nb_cfg.ip[1], nb_cfg.ip[2], nb_cfg.ip[3]);
    printf("NETBOOT: MAC %02X:%02X:%02X:%02X:%02X:%02X, IP %s, UDP %u\n",
           nb_cfg.mac[0], nb_cfg.mac[1], nb_cfg.mac[2], nb_cfg.mac[3], nb_cfg.mac[4], nb_cfg.mac[5],
           nb_ipstr, (unsigned)NB_PORT);
    nb_display("NETWORK FLASH MODE", nb_ipstr);

    if (!nb_eth_init(nb_cfg.mac))
    {
        printf("NETBOOT: Ethernet unavailable, rebooting in 5 s\n");
        nb_display("NETWORK FLASH MODE", "ETHERNET INIT FAILED");
        for (uint32_t i = 0; i < 50u; i++)
        {
            otaBoot_refreshWatchdog();
            HAL_Delay(100);
        }
        NVIC_SystemReset();
    }

    nb_core_init(&nb_cfg);
    t_enter = HAL_GetTick();

    for (;;)
    {
        uint32_t now = HAL_GetTick();
        uint16_t n;

        otaBoot_refreshWatchdog();

        if ((now - t_link) >= NETBOOT_LINK_POLL_MS)
        {
            int l = nb_eth_service();
            t_link = now;
            if (l != link)
            {
                link = l;
                printf("NETBOOT: link %s\n", l ? "up" : "down");
                nb_display("NETWORK FLASH MODE", l ? nb_ipstr : "WAITING FOR LINK");
            }
        }

        while ((n = nb_eth_poll(nb_frame, sizeof(nb_frame))) != 0u)
        {
            nb_core_input(nb_frame, n);
        }
        nb_core_poll();

        if (nb_core_client_bound() && !ever_bound)
        {
            ever_bound = true;
            printf("NETBOOT: host connected\n");
            nb_display("NETWORK FLASH MODE", "HOST CONNECTED");
        }

        if (!ever_bound && reason != NETBOOT_REASON_FAILED && (now - t_enter) > NETBOOT_IDLE_TIMEOUT_MS)
        {
            printf("NETBOOT: no host within %lu s, rebooting\n", (unsigned long)(NETBOOT_IDLE_TIMEOUT_MS / 1000u));
            HAL_Delay(10);
            NVIC_SystemReset();
        }
    }
}

/* ---- Portage du coeur ---------------------------------------------------- */

void nbp_send_frame(const uint8_t *frame, uint16_t len)
{
    (void)nb_eth_send(frame, len);
}

uint32_t nbp_tick_ms(void)
{
    return HAL_GetTick();
}

void nbp_get_info(nb_devinfo_t *info)
{
    ota_record_t rec;

    sys_identity_uid(info->uid);
    sys_identity_name(info->name);
    strncpy(info->bl_version, BL_VERSION, sizeof(info->bl_version) - 1u);
    if (ota_journal_read(&rec))
    {
        info->journal_phase = rec.phase;
        info->trial         = rec.trial_attempts;
        info->rollback      = rec.rollback_attempts;
        info->pending       = rec.pending_attempts;
    }
    info->reset_flags = otaBoot_resetCause();
    info->reason      = (uint8_t)nb_reason;
    info->boot_slot   = (running_sector_base() == NB_BL_SLOT_B) ? 1u : 0u;
}

/* Secteur d'ou ce bootloader s'execute : jamais efface ni ecrit. VTOR est pose
 * sur sa propre table des vecteurs des l'entree de main(), quel que soit le slot. */
static uint32_t running_sector_base(void)
{
    return SCB->VTOR & ~(FLASH_SECTOR_SIZE - 1u);
}

static bool flash_range_ok(uint32_t addr, uint32_t len)
{
    uint32_t self = running_sector_base();

    if (len == 0u || addr < NB_FLASH_START || addr >= NB_FLASH_END || len > NB_FLASH_END - addr)
    {
        return false;
    }
    /* intersection avec [self, self + secteur) */
    return (addr + len <= self) || (addr >= self + FLASH_SECTOR_SIZE);
}

/* Zones lisibles sans risque de faute de bus depuis le CM7. */
static bool read_range_ok(uint32_t addr, uint32_t len)
{
    static const struct { uint32_t base; uint32_t size; } zones[] = {
        {0x08000000u, 0x00200000u}, /* flash                  */
        {0x20000000u, 0x00020000u}, /* DTCM                   */
        {0x24000000u, 0x00080000u}, /* AXI SRAM               */
        {0x30000000u, 0x00048000u}, /* D2 SRAM1/2/3           */
        {0x38000000u, 0x00010000u}, /* D3 SRAM4 (logs, boite) */
    };
    if (len == 0u)
    {
        return false;
    }
    for (size_t i = 0; i < sizeof(zones) / sizeof(zones[0]); i++)
    {
        if (addr >= zones[i].base && addr < zones[i].base + zones[i].size &&
            len <= zones[i].base + zones[i].size - addr)
        {
            return true;
        }
    }
    return false;
}

int nbp_flash_erase(uint32_t addr, uint32_t len, uint32_t *fail_addr)
{
    uint32_t end;

    if (!flash_range_ok(addr, len) || (addr & (FLASH_SECTOR_SIZE - 1u)) != 0u)
    {
        *fail_addr = addr;
        return NB_E_RANGE;
    }
    end = addr + len;
    for (uint32_t a = addr; a < end; a += FLASH_SECTOR_SIZE)
    {
        uint32_t bank   = (a >= ADDR_FLASH_SECTOR_0_BANK2) ? FLASH_BANK_2 : FLASH_BANK_1;
        uint32_t sector = stm32Flash_getSector(a);

        otaBoot_refreshWatchdog();
        printf("NETBOOT: erase 0x%08lX (bank %u, sector %lu)\n",
               (unsigned long)a, (bank == FLASH_BANK_2) ? 2u : 1u, (unsigned long)sector);
        if (STM32Flash_erase_sector(bank, sector) != STM32FLASH_OK)
        {
            *fail_addr = a;
            return NB_E_FLASH;
        }
    }
    otaBoot_refreshWatchdog();
    return NB_OK;
}

/* Un mot de 32 octets : ignore s'il porte deja les donnees (retransmission),
 * refuse s'il n'est pas vierge, programme et relu sinon. */
static int program_word(uint32_t addr, const uint8_t *src)
{
    uint32_t tmp[8] __attribute__((aligned(32)));
    const uint8_t *cur = (const uint8_t *)addr;

    if (memcmp(cur, src, 32) == 0)
    {
        return NB_OK;
    }
    for (uint32_t i = 0; i < 32u; i++)
    {
        if (cur[i] != 0xFFu)
        {
            return NB_E_NOT_ERASED;
        }
    }
    memcpy(tmp, src, 32);
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr, (uint32_t)tmp) != HAL_OK)
    {
        return NB_E_FLASH;
    }
    if (memcmp(cur, tmp, 32) != 0)
    {
        return NB_E_VERIFY;
    }
    return NB_OK;
}

int nbp_flash_write(uint32_t addr, const uint8_t *data, uint32_t len, uint32_t *fail_addr)
{
    int st = NB_OK;

    if (!flash_range_ok(addr, len) || (addr & 31u) != 0u || (len & 31u) != 0u)
    {
        *fail_addr = addr;
        return NB_E_RANGE;
    }
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        *fail_addr = addr;
        return NB_E_FLASH;
    }
    for (uint32_t off = 0; off < len; off += 32u)
    {
        st = program_word(addr + off, data + off);
        if (st != NB_OK)
        {
            *fail_addr = addr + off;
            break;
        }
    }
    HAL_FLASH_Lock();
    return st;
}

int nbp_mem_read(uint32_t addr, uint8_t *out, uint32_t len)
{
    if (!read_range_ok(addr, len))
    {
        return NB_E_RANGE;
    }
    memcpy(out, (const void *)addr, len);
    return NB_OK;
}

int nbp_crc32(uint32_t addr, uint32_t len, uint32_t *crc)
{
    if (!read_range_ok(addr, len))
    {
        return NB_E_RANGE;
    }
    *crc = ota_crc32(OTA_CRC32_INIT, (const void *)addr, len);
    return NB_OK;
}

uint32_t nbp_log_read(uint16_t src, uint32_t since, uint8_t *out, uint16_t max, uint32_t *next)
{
    return log_ring_read((src == 1u) ? LOG_SRC_CM4 : LOG_SRC_CM7, since, out, max, next);
}

int nbp_journal_write(uint8_t phase, uint8_t trial, uint8_t rollback, uint8_t pending)
{
    bool ok;

    switch (phase)
    {
    case 0u:
        ok = ota_journal_clear();
        break;
    case OTA_PHASE_IDLE:
    case OTA_PHASE_PENDING:
    case OTA_PHASE_TRIAL:
    case OTA_PHASE_ROLLBACK:
    case OTA_PHASE_FAILED:
        ok = ota_journal_write((ota_phase_t)phase, trial, rollback, pending);
        break;
    default:
        return NB_E_ARG;
    }
    printf("NETBOOT: journal %s -> %s\n", ota_phase_str(phase), ok ? "written" : "FAILED");
    return ok ? NB_OK : NB_E_FLASH;
}

/* Bascule BOOT_ADD0 vers l'autre slot du bootloader. Le slot cible doit deja
 * porter une image plausible : pointeur de pile en RAM, vecteur de reset dans
 * le slot avec le bit Thumb. Prend effet au prochain reset ; l'ancien slot
 * reste intact, donc l'operation inverse est toujours possible depuis le
 * nouveau bootloader. */
int nbp_boot_select(uint32_t addr)
{
    FLASH_OBProgramInitTypeDef ob;
    const uint32_t sp = *(const volatile uint32_t *)addr;
    const uint32_t pc = *(const volatile uint32_t *)(addr + 4u);
    HAL_StatusTypeDef st;

    if (addr != NB_BL_SLOT_A && addr != NB_BL_SLOT_B)
    {
        return NB_E_ARG;
    }
    if (addr == running_sector_base())
    {
        return NB_OK; /* deja le slot actif */
    }
    if (sp < 0x20000000u || sp > 0x24080000u || (sp & 3u) != 0u ||
        pc < addr || pc >= addr + FLASH_SECTOR_SIZE || (pc & 1u) == 0u)
    {
        printf("NETBOOT: no bootable image at 0x%08lX (sp 0x%08lX, pc 0x%08lX)\n",
               (unsigned long)addr, (unsigned long)sp, (unsigned long)pc);
        return NB_E_VERIFY;
    }

    memset(&ob, 0, sizeof(ob));
    ob.OptionType = OPTIONBYTE_BOOTADD;
    ob.BootConfig = OB_BOOT_ADD0;
    ob.BootAddr0  = addr;

    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    st = HAL_FLASHEx_OBProgram(&ob);
    if (st == HAL_OK)
    {
        st = HAL_FLASH_OB_Launch();
    }
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

    printf("NETBOOT: BOOT_ADD0 -> 0x%08lX %s\n", (unsigned long)addr, (st == HAL_OK) ? "programmed" : "FAILED");
    return (st == HAL_OK) ? NB_OK : NB_E_FLASH;
}

void nbp_reboot(void)
{
    printf("NETBOOT: rebooting\n");
    HAL_Delay(20);
    NVIC_SystemReset();
}
