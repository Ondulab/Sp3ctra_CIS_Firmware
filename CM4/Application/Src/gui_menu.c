/**
 ******************************************************************************
 * @file           : gui_menu.c
 * @brief          : On-device setup menu (3 buttons, no host session needed)
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
 *
 * Standalone configuration (typically on a power bank, no network): while no
 * host session is bound (shared_feedback.link_state == 0), the OK button opens
 * a menu band at the top of the display. Like the host overlay, the band
 * squeezes the live CIS image into the rows below instead of masking it - so
 * the SCAN calibration pages keep the live image feedback on screen, and the
 * IMU calibration page shows the live accelerometer/gyroscope values.
 *
 * Physical layout (PCB order, default handedness = 1): SW3 alone LEFT of the
 * screen, SW1 and SW2 RIGHT of it, SW2 outermost. Navigation follows the
 * screen orientation (cis_handedness flips the panel 180 degrees, so the
 * roles swap with it):
 *   left button          : previous (scroll left)
 *   inner right button   : next (scroll right)
 *   outer right button   : validate / enter
 *
 * Structure: ROOT carousel (SCAN | IMU | NETWORK | GUI | EXIT) -> item list
 * per category -> value editor. Every change is posted to the CM7 through the
 * shared_var.menu_req_* mailbox and executed by the link task with the exact
 * same validation/side effects/persistence as a host SLP CFG_SET.
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "basetypes.h"
#include "globals.h"
#include "config.h"
#include "gui_config.h"

#include "ssd1362.h"
#include "sp3ctra_link.h"
#include "gui_interaction.h"
#include "gui_menu.h"

/* Private define ------------------------------------------------------------*/
#define MENU_COL_BG            (0)
#define MENU_COL_SEP           (6)
#define MENU_COL_DIM           (8)
#define MENU_COL_HI            (15)
#define MENU_CHAR_W            (8)      /* real advance of BOTH font faces */

#define MENU_BAND_H            (26)     /* root / cal / confirm pages */
#define MENU_BAND_ITEMS_H      (32)     /* item pages: tab row + 16 px value line */
#define MENU_BAND_IMU_H        (40)     /* IMU calibration page (2 live rows) */
#define MENU_SLIDE_OPEN_MS     (140.0f)
#define MENU_SLIDE_CLOSE_MS    (400.0f)

#define MENU_IDLE_CLOSE_MS     (30000U) /* no button -> close (except while calibrating) */
#define MENU_TOAST_MS          (1500U)
#define MENU_RESULT_MS         (2000U)  /* DONE / FAILED shown on the cal pages */

#define MENU_REPEAT_DELAY_MS   (450U)   /* hold a nav button ... */
#define MENU_REPEAT_MS         (70U)    /* ... then it repeats at this rate */

#define MENU_ACK_TIMEOUT_MS    (3000U)  /* CM7 silent (link task not up yet) */
#define MENU_IMU_CAL_TIMEOUT_MS (10000U)
#define MENU_BP_CAL_TIMEOUT_MS (30000U)
#define MENU_RESET_TIMEOUT_MS  (60000U) /* QSPI format takes several seconds */

/* Private types --------------------------------------------------------------*/
typedef enum
{
    MENU_CLOSED = 0,
    MENU_ROOT,
    MENU_ITEMS,
    MENU_EDIT,
    MENU_CAL_SCAN,
    MENU_CAL_BP,
    MENU_CAL_IMU,
    MENU_CONFIRM_RESET,
} MenuState;

typedef enum
{
    CAL_IDLE = 0,
    CAL_RUNNING,
    CAL_DONE,
    CAL_FAILED,
} CalPhase;

typedef enum
{
    ROLE_NONE = 0,
    ROLE_PREV,
    ROLE_NEXT,
    ROLE_OK,
} ButtonRole;

typedef enum
{
    MI_CFG = 0,       /* editable shared_config field, applied by the CM7 */
    MI_ACTION_CAL_SCAN,
    MI_ACTION_CAL_BP,
    MI_ACTION_CAL_IMU,
    MI_ACTION_FACTORY_RESET,
    MI_BACK,
} MenuItemKind;

typedef enum
{
    VT_NONE = 0,
    VT_INT,           /* integer, vmin..vmax, vstep */
    VT_BOOL,          /* OFF / ON */
    VT_ENUM,          /* enum_values[] / enum_labels[] */
    VT_F100,          /* float edited as value x100 (2 decimals) */
    VT_IP,            /* 4 octets, edited one by one */
} MenuValueType;

typedef struct
{
    const char *label;
    MenuItemKind kind;
    uint16_t cfg_id;
    MenuValueType vtype;
    int32_t vmin;
    int32_t vmax;
    int32_t vstep;
    const int32_t *enum_values;
    const char *const *enum_labels;
    uint8_t enum_count;
    const char *unit;
} MenuItem;

/* Menu tables ----------------------------------------------------------------*/
static const int32_t dpi_values[] = {200, 400};
static const char *const dpi_labels[] = {"200", "400"};

static const int32_t hand_values[] = {0, 1};
static const char *const hand_labels[] = {"LEFT", "RIGHT"};

static const int32_t gyro_values[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const char *const gyro_labels[] = {"2000", "1000", "500", "250", "125", "62.5", "31.2", "15.6"};

static const int32_t accel_values[] = {0, 1, 2, 3};
static const char *const accel_labels[] = {"16", "8", "4", "2"};

static const MenuItem scan_items[] = {
    {"CALIBRATE",    MI_ACTION_CAL_SCAN, 0, VT_NONE, 0, 0, 0, NULL, NULL, 0, ""},
    {"BLACK CAL",    MI_ACTION_CAL_BP,   0, VT_NONE, 0, 0, 0, NULL, NULL, 0, ""},
    {"DPI",          MI_CFG, SLP_CFG_DPI,          VT_ENUM, 0, 0, 0, dpi_values, dpi_labels, 2, ""},
    {"OVERSAMPLING", MI_CFG, SLP_CFG_OVERSAMPLING, VT_INT, 1, 32, 1, NULL, NULL, 0, "X"},
    {"BLACK POINT",  MI_CFG, SLP_CFG_BLACK_POINT,  VT_INT, 0, 200, 5, NULL, NULL, 0, ""},
    {"HANDEDNESS",   MI_CFG, SLP_CFG_HANDEDNESS,   VT_ENUM, 0, 0, 0, hand_values, hand_labels, 2, ""},
    {"BACK",         MI_BACK, 0, VT_NONE, 0, 0, 0, NULL, NULL, 0, ""},
};

static const MenuItem imu_items[] = {
    {"CALIBRATE",    MI_ACTION_CAL_IMU, 0, VT_NONE, 0, 0, 0, NULL, NULL, 0, ""},
    {"GYRO RANGE",   MI_CFG, SLP_CFG_GYRO_FS,  VT_ENUM, 0, 0, 0, gyro_values, gyro_labels, 8, "DPS"},
    {"ACCEL RANGE",  MI_CFG, SLP_CFG_ACCEL_FS, VT_ENUM, 0, 0, 0, accel_values, accel_labels, 4, "G"},
    {"BACK",         MI_BACK, 0, VT_NONE, 0, 0, 0, NULL, NULL, 0, ""},
};

static const MenuItem network_items[] = {
    {"IP",           MI_CFG, SLP_CFG_NET_IP,      VT_IP, 0, 0, 0, NULL, NULL, 0, ""},
    {"NETMASK",      MI_CFG, SLP_CFG_NET_MASK,    VT_IP, 0, 0, 0, NULL, NULL, 0, ""},
    {"GATEWAY",      MI_CFG, SLP_CFG_NET_GW,      VT_IP, 0, 0, 0, NULL, NULL, 0, ""},
    {"DEST IP",      MI_CFG, SLP_CFG_NET_DEST_IP, VT_IP, 0, 0, 0, NULL, NULL, 0, ""},
    {"STREAM PORT",  MI_CFG, SLP_CFG_STREAM_PORT, VT_INT, 1, 65535, 1, NULL, NULL, 0, ""},
    {"LINK PORT",    MI_CFG, SLP_CFG_LINK_PORT,   VT_INT, 1, 65535, 1, NULL, NULL, 0, ""},
    {"STREAM UNBND", MI_CFG, SLP_CFG_STREAM_WHEN_UNBOUND, VT_BOOL, 0, 1, 1, NULL, NULL, 0, ""},
    {"BACK",         MI_BACK, 0, VT_NONE, 0, 0, 0, NULL, NULL, 0, ""},
};

static const MenuItem gui_items[] = {
    {"SHOW IMU",     MI_CFG, SLP_CFG_GUI_SHOW_IMU,   VT_BOOL, 0, 1, 1, NULL, NULL, 0, ""},
    {"INVERT",       MI_CFG, SLP_CFG_GUI_INVERT,     VT_BOOL, 0, 1, 1, NULL, NULL, 0, ""},
    {"SAVER TIME",   MI_CFG, SLP_CFG_SCREENSAVER_S,  VT_INT, 1, 1000, 1, NULL, NULL, 0, "S"},
    {"MOTION ACC",   MI_CFG, SLP_CFG_MOTION_THR_ACC, VT_F100, 1, 100, 1, NULL, NULL, 0, "G"},
    {"MOTION GYRO",  MI_CFG, SLP_CFG_MOTION_THR_GYRO, VT_F100, 50, 1000, 10, NULL, NULL, 0, "DPS"},
    {"BACK",         MI_BACK, 0, VT_NONE, 0, 0, 0, NULL, NULL, 0, ""},
};

static const MenuItem system_items[] = {
    {"FACTORY RESET", MI_ACTION_FACTORY_RESET, 0, VT_NONE, 0, 0, 0, NULL, NULL, 0, ""},
    {"BACK",          MI_BACK, 0, VT_NONE, 0, 0, 0, NULL, NULL, 0, ""},
};

typedef struct
{
    const char *name;
    const MenuItem *items;
    uint8_t count;
} MenuCategory;

static const MenuCategory categories[] = {
    {"SCAN",    scan_items,    (uint8_t)(sizeof(scan_items) / sizeof(scan_items[0]))},
    {"IMU",     imu_items,     (uint8_t)(sizeof(imu_items) / sizeof(imu_items[0]))},
    {"NETWORK", network_items, (uint8_t)(sizeof(network_items) / sizeof(network_items[0]))},
    {"GUI",     gui_items,     (uint8_t)(sizeof(gui_items) / sizeof(gui_items[0]))},
    {"SYSTEM",  system_items,  (uint8_t)(sizeof(system_items) / sizeof(system_items[0]))},
    {"EXIT",    NULL,          0},
};
#define MENU_CATEGORY_COUNT   ((uint8_t)(sizeof(categories) / sizeof(categories[0])))

/* Private variables ----------------------------------------------------------*/
static MenuState menu_state = MENU_CLOSED;
static MenuState draw_state = MENU_ROOT;   /* content shown while the band slides out */
static uint8_t cat_idx = 0;
static uint8_t item_idx = 0;

/* Editor */
static int32_t edit_val = 0;
static uint8_t edit_ip[4] = {0};
static uint8_t edit_octet = 0;

/* One request in flight at a time */
static uint8_t  req_waiting = 0;
static uint32_t req_seq = 0;
static uint32_t req_sent_tick = 0;
static uint32_t req_timeout_ms = MENU_ACK_TIMEOUT_MS;

/* Toast on the band's first line */
static char toast[24] = {0};
static uint32_t toast_tick = 0;

/* Calibration pages */
static CalPhase cal_phase = CAL_IDLE;
static uint32_t cal_tick = 0;
static uint8_t  bp_running_seen = 0;

/* Slide animation (same integrator as the host overlay) */
static float    anim_h = 0.0f;
static int32_t  band_h = 0;
static uint32_t anim_tick = 0;
static uint8_t  anim_inited = 0;

/* Input */
static uint32_t last_input_tick = 0;
static ButtonRole held_role = ROLE_NONE;
static uint32_t next_repeat_tick = 0;
static uint32_t repeat_count = 0;

/* Button roles ----------------------------------------------------------------
 * PCB order left->right: SW3 | screen | SW1 SW2. Handedness 1 (default) shows
 * that orientation; handedness 0 flips the screen 180 degrees. Validate (and
 * menu entry) is SW1, the inner button of the pair - next to the screen in both
 * orientations; the two OUTER buttons scroll. Direction fixed after on-device
 * test 2026-09-02 ("on est inverse"): swapped from the first guess, so the
 * left-hand outer button moves the selection LEFT and the right-hand one
 * moves it RIGHT. */
static buttonIdTypeDef role_button(ButtonRole role)
{
    const uint8_t hand = shared_config.cis_handedness ? 1U : 0U;
    switch (role)
    {
        case ROLE_PREV: return hand ? SW2 : SW3;
        case ROLE_NEXT: return hand ? SW3 : SW2;
        case ROLE_OK:
        default:        return SW1;
    }
}

static ButtonRole button_role(buttonIdTypeDef id)
{
    if (id == role_button(ROLE_OK))   return ROLE_OK;
    if (id == role_button(ROLE_PREV)) return ROLE_PREV;
    if (id == role_button(ROLE_NEXT)) return ROLE_NEXT;
    return ROLE_NONE;
}

/* Value access ---------------------------------------------------------------*/

static volatile uint8_t *ip_field(uint16_t cfg_id)
{
    switch (cfg_id)
    {
        case SLP_CFG_NET_IP:      return shared_config.network_ip;
        case SLP_CFG_NET_MASK:    return shared_config.network_netmask;
        case SLP_CFG_NET_GW:      return shared_config.network_gw;
        case SLP_CFG_NET_DEST_IP: return shared_config.network_dest_ip;
        default:                  return NULL;
    }
}

/** Current value of a scalar item, in its edited integer representation. */
static int32_t item_readValue(const MenuItem *it)
{
    switch (it->cfg_id)
    {
        case SLP_CFG_DPI:             return (int32_t)shared_config.cis_dpi;
        case SLP_CFG_OVERSAMPLING:    return (int32_t)shared_config.cis_oversampling;
        case SLP_CFG_BLACK_POINT:     return (int32_t)shared_config.cis_black_point;
        case SLP_CFG_HANDEDNESS:      return (int32_t)shared_config.cis_handedness;
        case SLP_CFG_GYRO_FS:         return (int32_t)shared_config.imu_gyro_sensitivity;
        case SLP_CFG_ACCEL_FS:        return (int32_t)shared_config.imu_accel_sensitivity;
        case SLP_CFG_GUI_SHOW_IMU:    return (int32_t)shared_config.gui_show_imu;
        case SLP_CFG_GUI_INVERT:      return (int32_t)shared_config.gui_invert_cis_image;
        case SLP_CFG_SCREENSAVER_S:   return (int32_t)shared_config.screensaver_timeout_sec;
        case SLP_CFG_STREAM_PORT:     return (int32_t)shared_config.network_udp_port;
        case SLP_CFG_STREAM_WHEN_UNBOUND: return (int32_t)shared_config.stream_when_unbound;
        case SLP_CFG_LINK_PORT:       return (int32_t)shared_config.network_link_port;
        case SLP_CFG_MOTION_THR_ACC:  return (int32_t)(shared_config.motion_threshold_acc * 100.0f + 0.5f);
        case SLP_CFG_MOTION_THR_GYRO: return (int32_t)(shared_config.motion_threshold_gyro * 100.0f + 0.5f);
        default:                      return 0;
    }
}

static uint8_t enum_indexOf(const MenuItem *it, int32_t value)
{
    for (uint8_t i = 0; i < it->enum_count; i++)
    {
        if (it->enum_values[i] == value)
        {
            return i;
        }
    }
    return 0;
}

/* Request mailbox -------------------------------------------------------------*/

static void menu_post(uint32_t kind, uint32_t id, uint32_t value, uint32_t timeout_ms)
{
    shared_var.menu_req_kind = kind;
    shared_var.menu_req_id = id;
    shared_var.menu_req_value = value;
    shared_var.menu_req_seq = shared_var.menu_req_seq + 1U;   /* posted last */
    req_seq = shared_var.menu_req_seq;
    req_sent_tick = HAL_GetTick();
    req_timeout_ms = timeout_ms;
    req_waiting = 1;
}

static void menu_sendCfg(const MenuItem *it)
{
    uint32_t value;
    if (it->vtype == VT_IP)
    {
        value = (uint32_t)edit_ip[0] | ((uint32_t)edit_ip[1] << 8) |
                ((uint32_t)edit_ip[2] << 16) | ((uint32_t)edit_ip[3] << 24);
    }
    else if (it->vtype == VT_F100)
    {
        const float f = (float)edit_val / 100.0f;
        memcpy(&value, &f, sizeof(value));
    }
    else
    {
        value = (uint32_t)edit_val;
    }
    menu_post(MENU_REQ_CFG_SET, it->cfg_id, value, MENU_ACK_TIMEOUT_MS);
}

static void menu_toast(const char *text)
{
    snprintf(toast, sizeof(toast), "%s", text);
    toast_tick = HAL_GetTick();
}

/* Formatting ------------------------------------------------------------------*/

/** value x100 -> "d.dd" (no float printf on this core). */
static void fmt_f100(char *dst, uint32_t n, int32_t v100)
{
    const char *sign = (v100 < 0) ? "-" : "";
    if (v100 < 0)
    {
        v100 = -v100;
    }
    snprintf(dst, n, "%s%ld.%02ld", sign, (long)(v100 / 100), (long)(v100 % 100));
}

/** IMU cell: value x100 -> "+d.dd" fixed sign, for the live rows. */
static void fmt_imu(char *dst, uint32_t n, float v)
{
    int32_t v100 = (int32_t)(v * 100.0f + ((v >= 0.0f) ? 0.5f : -0.5f));
    const char sign = (v100 < 0) ? '-' : '+';
    if (v100 < 0)
    {
        v100 = -v100;
    }
    if (v100 > 99999)
    {
        v100 = 99999;
    }
    snprintf(dst, n, "%c%ld.%02ld", sign, (long)(v100 / 100), (long)(v100 % 100));
}

/** Displayed string for an item value (current or edited). */
static void item_formatValue(const MenuItem *it, int32_t value, const uint8_t ip[4], char *dst, uint32_t n)
{
    switch (it->vtype)
    {
        case VT_BOOL:
            snprintf(dst, n, "%s", value ? "ON" : "OFF");
            break;
        case VT_ENUM:
            snprintf(dst, n, "%s%s", it->enum_labels[enum_indexOf(it, value)], it->unit);
            break;
        case VT_F100:
        {
            char num[16];
            fmt_f100(num, sizeof(num), value);
            snprintf(dst, n, "%s%s", num, it->unit);
            break;
        }
        case VT_IP:
            snprintf(dst, n, "%03u.%03u.%03u.%03u", ip[0], ip[1], ip[2], ip[3]);
            break;
        case VT_INT:
        default:
            snprintf(dst, n, "%ld%s", (long)value, it->unit);
            break;
    }
}

/* State helpers ---------------------------------------------------------------*/

static const MenuCategory *cur_cat(void)
{
    return &categories[cat_idx];
}

static const MenuItem *cur_item(void)
{
    return &cur_cat()->items[item_idx];
}

static int32_t menu_fullHeight(MenuState s)
{
    if (s == MENU_CAL_IMU)
    {
        return MENU_BAND_IMU_H;
    }
    if (s == MENU_ITEMS || s == MENU_EDIT)
    {
        return MENU_BAND_ITEMS_H;
    }
    return MENU_BAND_H;
}

uint8_t gui_menu_isActive(void)
{
    return (menu_state != MENU_CLOSED) ? 1U : 0U;
}

buttonIdTypeDef gui_menu_okButton(void)
{
    return role_button(ROLE_OK);
}

static void menu_close(void)
{
    menu_state = MENU_CLOSED;
    held_role = ROLE_NONE;
}

static void menu_enterEdit(void)
{
    const MenuItem *it = cur_item();
    if (it->vtype == VT_IP)
    {
        volatile uint8_t *src = ip_field(it->cfg_id);
        for (uint8_t i = 0; i < 4; i++)
        {
            edit_ip[i] = src ? src[i] : 0;
        }
        edit_octet = 0;
    }
    else
    {
        edit_val = item_readValue(it);
    }
    menu_state = MENU_EDIT;
}

/** +/-1 nav step on the edited value; repeat_count accelerates wide ranges. */
static void menu_editStep(const MenuItem *it, int32_t dir)
{
    if (it->vtype == VT_IP)
    {
        int32_t v = (int32_t)edit_ip[edit_octet] + dir * ((repeat_count > 20U) ? 10 : 1);
        while (v < 0)
        {
            v += 256;
        }
        edit_ip[edit_octet] = (uint8_t)(v % 256);
        return;
    }
    if (it->vtype == VT_ENUM)
    {
        int32_t idx = (int32_t)enum_indexOf(it, edit_val) + dir;
        if (idx < 0)
        {
            idx = it->enum_count - 1;
        }
        if (idx >= (int32_t)it->enum_count)
        {
            idx = 0;
        }
        edit_val = it->enum_values[idx];
        return;
    }

    int32_t mult = 1;
    const int32_t range = it->vmax - it->vmin;
    if (range > 300)
    {
        mult = (repeat_count > 60U) ? 100 : (repeat_count > 20U) ? 10 : 1;
    }
    edit_val += dir * it->vstep * mult;
    if (edit_val < it->vmin)
    {
        edit_val = it->vmin;
    }
    if (edit_val > it->vmax)
    {
        edit_val = it->vmax;
    }
}

/* Actions ---------------------------------------------------------------------*/

static void menu_action(ButtonRole role)
{
    const int32_t dir = (role == ROLE_NEXT) ? 1 : -1;

    switch (menu_state)
    {
        case MENU_ROOT:
            if (role == ROLE_OK)
            {
                if (categories[cat_idx].items == NULL)   /* EXIT */
                {
                    menu_close();
                }
                else
                {
                    item_idx = 0;
                    menu_state = MENU_ITEMS;
                }
            }
            else
            {
                /* Tabs: hard stops at both ends, no wrap-around. */
                if (dir < 0 && cat_idx > 0U)
                {
                    cat_idx--;
                }
                else if (dir > 0 && cat_idx < (uint8_t)(MENU_CATEGORY_COUNT - 1U))
                {
                    cat_idx++;
                }
            }
            break;

        case MENU_ITEMS:
            if (role == ROLE_OK)
            {
                if (req_waiting)
                {
                    break;   /* one request at a time */
                }
                switch (cur_item()->kind)
                {
                    case MI_BACK:
                        menu_state = MENU_ROOT;
                        break;
                    case MI_ACTION_CAL_SCAN:
                        menu_state = MENU_CAL_SCAN;
                        break;
                    case MI_ACTION_CAL_BP:
                        cal_phase = CAL_IDLE;
                        bp_running_seen = 0;
                        menu_state = MENU_CAL_BP;
                        break;
                    case MI_ACTION_CAL_IMU:
                        cal_phase = CAL_IDLE;
                        menu_state = MENU_CAL_IMU;
                        break;
                    case MI_ACTION_FACTORY_RESET:
                        cal_phase = CAL_IDLE;
                        menu_state = MENU_CONFIRM_RESET;
                        break;
                    case MI_CFG:
                    default:
                        menu_enterEdit();
                        break;
                }
            }
            else
            {
                /* Item list: hard stops too ("n/N" shows the position). */
                if (dir < 0 && item_idx > 0U)
                {
                    item_idx--;
                }
                else if (dir > 0 && item_idx < (uint8_t)(cur_cat()->count - 1U))
                {
                    item_idx++;
                }
            }
            break;

        case MENU_EDIT:
            if (role == ROLE_OK)
            {
                const MenuItem *it = cur_item();
                if (it->vtype == VT_IP && edit_octet < 3U)
                {
                    edit_octet++;
                }
                else
                {
                    menu_sendCfg(it);
                    menu_state = MENU_ITEMS;
                }
            }
            else
            {
                menu_editStep(cur_item(), dir);
            }
            break;

        case MENU_CAL_SCAN:
            if (role == ROLE_OK)
            {
                /* Hand over to the stock modal calibration flow: the next
                 * gui_interractiveMenu() frame runs gui_startCalibration()
                 * while the CM7 executes the white/black sequence. Snap the
                 * band shut - the modal takes the whole panel anyway. */
                menu_close();
                anim_h = 0.0f;
                band_h = 0;
                shared_var.cis_cal_state = CIS_CAL_REQUESTED;
            }
            else
            {
                menu_state = MENU_ITEMS;
            }
            break;

        case MENU_CAL_BP:
            if (role == ROLE_OK)
            {
                if (cal_phase == CAL_IDLE && !req_waiting)
                {
                    bp_running_seen = 0;
                    cal_tick = HAL_GetTick();
                    cal_phase = CAL_RUNNING;
                    menu_post(MENU_REQ_BLACKPOINT_CAL, 0, 0, MENU_ACK_TIMEOUT_MS);
                }
            }
            else if (cal_phase != CAL_RUNNING)
            {
                menu_state = MENU_ITEMS;
            }
            break;

        case MENU_CAL_IMU:
            if (role == ROLE_OK)
            {
                if (cal_phase == CAL_IDLE && !req_waiting)
                {
                    cal_tick = HAL_GetTick();
                    cal_phase = CAL_RUNNING;
                    menu_post(MENU_REQ_IMU_CAL, 0, 0, MENU_IMU_CAL_TIMEOUT_MS);
                }
            }
            else if (cal_phase != CAL_RUNNING)
            {
                menu_state = MENU_ITEMS;
            }
            break;

        case MENU_CONFIRM_RESET:
            if (role == ROLE_OK)
            {
                /* Second deliberate press on a dedicated screen: erases the
                 * whole QSPI (config, calibrations, admin password), then the
                 * device reboots on defaults. */
                if (cal_phase == CAL_IDLE && !req_waiting)
                {
                    cal_tick = HAL_GetTick();
                    cal_phase = CAL_RUNNING;
                    menu_post(MENU_REQ_FACTORY_RESET, 0, 0, MENU_RESET_TIMEOUT_MS);
                }
            }
            else if (cal_phase != CAL_RUNNING)
            {
                menu_state = MENU_ITEMS;
            }
            break;

        default:
            break;
    }
}

/* Input -----------------------------------------------------------------------*/

void gui_menu_onButton(buttonIdTypeDef id, buttonStateTypeDef state)
{
    const ButtonRole role = button_role(id);

    if (state == SWITCH_RELEASED)
    {
        if (role == held_role)
        {
            held_role = ROLE_NONE;
        }
        return;
    }

    last_input_tick = HAL_GetTick();

    if (menu_state == MENU_CLOSED)
    {
        /* OK opens the menu - only while no host session owns the buttons. */
        if (role == ROLE_OK && shared_feedback.link_state == 0U)
        {
            cat_idx = 0;
            menu_state = MENU_ROOT;
        }
        return;
    }

    if (role == ROLE_PREV || role == ROLE_NEXT)
    {
        held_role = role;
        next_repeat_tick = last_input_tick + MENU_REPEAT_DELAY_MS;
        repeat_count = 0;
    }
    menu_action(role);
}

/* Tick (state advance) --------------------------------------------------------*/

static void menu_finishRequest(uint32_t result)
{
    if (menu_state == MENU_CAL_IMU && cal_phase == CAL_RUNNING)
    {
        cal_phase = (result == 0U) ? CAL_DONE : CAL_FAILED;
        cal_tick = HAL_GetTick();
        return;
    }
    if (menu_state == MENU_CONFIRM_RESET && cal_phase == CAL_RUNNING)
    {
        /* On success the CM7 reboots the device ~500 ms later: DONE shows briefly. */
        cal_phase = (result & SLP_CFG_F_REJECTED) ? CAL_FAILED : CAL_DONE;
        cal_tick = HAL_GetTick();
        return;
    }
    if (menu_state == MENU_CAL_BP)
    {
        /* Ack only means "started": progress is tracked via menu_bp_cal_state. */
        if (result != 0U)
        {
            cal_phase = CAL_FAILED;
            cal_tick = HAL_GetTick();
        }
        return;
    }

    if (result & SLP_CFG_F_REJECTED)
    {
        menu_toast("REJECTED");
    }
    else if (result & SLP_CFG_F_REBOOT)
    {
        menu_toast("SAVED - REBOOT...");
    }
    else if (result & SLP_CFG_F_UNKNOWN)
    {
        menu_toast("UNSUPPORTED");
    }
    else
    {
        menu_toast("SAVED");
    }
}

static void menu_tick(void)
{
    const uint32_t now = HAL_GetTick();

    /* A host session takes the buttons back. */
    if (menu_state != MENU_CLOSED && shared_feedback.link_state != 0U)
    {
        menu_close();
    }

    /* Request acknowledgement / timeout */
    if (req_waiting)
    {
        if (shared_var.menu_req_done_seq == req_seq)
        {
            req_waiting = 0;
            menu_finishRequest(shared_var.menu_req_result);
        }
        else if ((now - req_sent_tick) > req_timeout_ms)
        {
            req_waiting = 0;
            if ((menu_state == MENU_CAL_IMU || menu_state == MENU_CAL_BP ||
                 menu_state == MENU_CONFIRM_RESET) && cal_phase == CAL_RUNNING)
            {
                cal_phase = CAL_FAILED;
                cal_tick = now;
            }
            else
            {
                menu_toast("NO REPLY");
            }
        }
    }

    /* Black point calibration progress (mirrored from the CM7) */
    if (menu_state == MENU_CAL_BP && cal_phase == CAL_RUNNING && !req_waiting)
    {
        const uint32_t bp = shared_var.menu_bp_cal_state;
        if (bp == 1U)
        {
            bp_running_seen = 1;
        }
        else if (bp_running_seen && (bp == 2U || bp == 3U))
        {
            cal_phase = (bp == 2U) ? CAL_DONE : CAL_FAILED;
            cal_tick = now;
        }
        else if ((now - cal_tick) > MENU_BP_CAL_TIMEOUT_MS)
        {
            cal_phase = CAL_FAILED;
            cal_tick = now;
        }
    }

    /* Calibration/reset result screens return to idle */
    if ((menu_state == MENU_CAL_BP || menu_state == MENU_CAL_IMU ||
         menu_state == MENU_CONFIRM_RESET) &&
        (cal_phase == CAL_DONE || cal_phase == CAL_FAILED) &&
        (now - cal_tick) > MENU_RESULT_MS)
    {
        cal_phase = CAL_IDLE;
    }

    /* Auto-repeat of a held nav button */
    if (menu_state != MENU_CLOSED && held_role != ROLE_NONE)
    {
        if (!gui_button_isPressed(role_button(held_role)))
        {
            held_role = ROLE_NONE;
        }
        else if ((int32_t)(now - next_repeat_tick) >= 0)
        {
            repeat_count++;
            next_repeat_tick = now + MENU_REPEAT_MS;
            last_input_tick = now;
            menu_action(held_role);
        }
    }

    /* Idle close (never while a calibration or the factory reset runs) */
    if (menu_state != MENU_CLOSED &&
        !(cal_phase == CAL_RUNNING &&
          (menu_state == MENU_CAL_BP || menu_state == MENU_CAL_IMU ||
           menu_state == MENU_CONFIRM_RESET)) &&
        (now - last_input_tick) > MENU_IDLE_CLOSE_MS)
    {
        menu_close();
    }
}

/* Band geometry ---------------------------------------------------------------*/

uint32_t gui_menu_reservedTop(void)
{
    const uint32_t now = HAL_GetTick();

    if (!anim_inited)
    {
        anim_tick = now;
        anim_inited = 1;
    }

    menu_tick();

    if (menu_state != MENU_CLOSED)
    {
        draw_state = menu_state;   /* content keeps drawing while sliding out */
    }

    const int32_t target = (menu_state != MENU_CLOSED) ? menu_fullHeight(menu_state) : 0;

    uint32_t dt = now - anim_tick;
    anim_tick = now;
    if (dt > 100U)
    {
        dt = 100U;
    }

    const float ref = (float)menu_fullHeight(draw_state);
    if (anim_h < (float)target)
    {
        anim_h += ref * (float)dt / MENU_SLIDE_OPEN_MS;
        if (anim_h > (float)target)
        {
            anim_h = (float)target;
        }
    }
    else if (anim_h > (float)target)
    {
        anim_h -= ref * (float)dt / MENU_SLIDE_CLOSE_MS;
        if (anim_h < (float)target)
        {
            anim_h = (float)target;
        }
    }

    band_h = (int32_t)(anim_h + 0.5f);
    if (band_h < 0)
    {
        band_h = 0;
    }
    return (uint32_t)band_h;
}

/* Rendering -------------------------------------------------------------------*/

static void draw_small(int32_t x, int32_t y, const char *text, uint8_t col, int32_t clip)
{
    ssd1362_drawStringClipped(x, y, text, col, 8, 0, clip);
}

static void draw_big(int32_t x, int32_t y, const char *text, uint8_t col, int32_t clip)
{
    ssd1362_drawStringClipped(x, y, text, col, 16, 0, clip);
}

static void draw_small_centered(int32_t y, const char *text, uint8_t col, int32_t clip)
{
    draw_small((DISPLAY_WIDTH - (int32_t)strlen(text) * MENU_CHAR_W) / 2, y, text, col, clip);
}

static void draw_big_centered(int32_t y, const char *text, uint8_t col, int32_t clip)
{
    draw_big((DISPLAY_WIDTH - (int32_t)strlen(text) * MENU_CHAR_W) / 2, y, text, col, clip);
}

/** First line of the band: left/right captions, replaced by an active toast. */
static void draw_header(const char *left, const char *right, int32_t y, int32_t clip)
{
    if (toast[0] != '\0' && (HAL_GetTick() - toast_tick) < MENU_TOAST_MS)
    {
        draw_small_centered(y, toast, MENU_COL_HI, clip);
        return;
    }
    if (left != NULL)
    {
        draw_small(2, y, left, MENU_COL_DIM, clip);
    }
    if (right != NULL)
    {
        draw_small(DISPLAY_WIDTH - 2 - (int32_t)strlen(right) * MENU_CHAR_W, y, right, MENU_COL_DIM, clip);
    }
}

/* Short tab captions - the full category name titles the item pages. */
static const char *const cat_tabs[] = {"SCAN", "IMU", "NET", "GUI", "SYS", "EXIT"};

/** Row of tabs, the selected one boxed (inverted). When everything fits the
 *  row is centred; otherwise a window around the selection is shown, with dim
 *  chevrons at the edges hinting at the hidden tabs. No wrap-around. */
static void draw_tabs(const char *const *labels, uint8_t count, uint8_t sel, int32_t y, int32_t clip)
{
    const int32_t gap = 12;
    int32_t w[16];
    int32_t total = -gap;

    if (count > 16U)
    {
        count = 16U;
    }
    if (count == 0U || sel >= count)
    {
        return;
    }
    for (uint8_t i = 0; i < count; i++)
    {
        w[i] = (int32_t)strlen(labels[i]) * MENU_CHAR_W;
        total += w[i] + gap;
    }

    uint8_t first = sel;
    uint8_t last = sel;
    int32_t used = w[sel];
    if (total <= 252)
    {
        first = 0;
        last = (uint8_t)(count - 1U);
        used = total;
    }
    else
    {
        /* Grow the window around the selection, one tab per side per pass,
         * inside the room the edge chevrons leave. */
        bool grown = true;
        while (grown)
        {
            grown = false;
            if ((uint8_t)(last + 1U) < count && used + gap + w[last + 1] <= 232)
            {
                last++;
                used += gap + w[last];
                grown = true;
            }
            if (first > 0U && used + gap + w[first - 1] <= 232)
            {
                first--;
                used += gap + w[first];
                grown = true;
            }
        }
    }

    int32_t x = (DISPLAY_WIDTH - used) / 2;
    for (uint8_t i = first; i <= last; i++)
    {
        if (i == sel)
        {
            int32_t bx1 = x - 3;
            int32_t by1 = y - 3;
            int32_t by2 = y + 10;
            if (bx1 < 0)
            {
                bx1 = 0;
            }
            if (by1 < 0)
            {
                by1 = 0;
            }
            if (by2 > clip)
            {
                by2 = clip;
            }
            if (by2 >= by1)
            {
                ssd1362_fillRect((uint16_t)bx1, (uint16_t)by1, (uint16_t)(x + w[i] + 2), (uint16_t)by2, MENU_COL_HI, false);
            }
            draw_small(x, y, labels[i], 0, clip);
        }
        else
        {
            draw_small(x, y, labels[i], MENU_COL_DIM, clip);
        }
        x += w[i] + gap;
    }

    if (first > 0U)
    {
        draw_small(0, y, "<", MENU_COL_DIM, clip);
    }
    if ((uint8_t)(last + 1U) < count)
    {
        draw_small(DISPLAY_WIDTH - MENU_CHAR_W, y, ">", MENU_COL_DIM, clip);
    }
}

static void draw_root(int32_t y_shift, int32_t clip)
{
    draw_header("SETUP", NULL, 1 + y_shift, clip);
    draw_tabs(cat_tabs, MENU_CATEGORY_COUNT, cat_idx, 13 + y_shift, clip);
}

static void draw_itemValue(const MenuItem *it, uint8_t editing, int32_t y, int32_t clip)
{
    char value[24] = {0};

    if (req_waiting && !editing && it->kind == MI_CFG)
    {
        snprintf(value, sizeof(value), "...");
    }
    else if (it->kind == MI_CFG)
    {
        if (editing)
        {
            item_formatValue(it, edit_val, edit_ip, value, sizeof(value));
        }
        else
        {
            volatile uint8_t *src = ip_field(it->cfg_id);
            uint8_t cur_ip[4] = {0};
            if (src != NULL)
            {
                for (uint8_t i = 0; i < 4; i++)
                {
                    cur_ip[i] = src[i];
                }
            }
            item_formatValue(it, item_readValue(it), cur_ip, value, sizeof(value));
        }
    }
    else
    {
        return;   /* actions and BACK carry no value */
    }

    const int32_t value_x = DISPLAY_WIDTH - 2 - (int32_t)strlen(value) * MENU_CHAR_W;

    if (editing && it->vtype == VT_IP)
    {
        /* Highlight only the octet being edited. */
        const int32_t oct_x = value_x + (int32_t)edit_octet * 4 * MENU_CHAR_W;
        const int32_t box_y1 = y - 1;
        int32_t box_y2 = y + 16;
        if (box_y2 > clip)
        {
            box_y2 = clip;
        }
        if (box_y2 >= 0 && box_y1 <= clip)
        {
            ssd1362_fillRect((uint16_t)(oct_x - 1), (uint16_t)((box_y1 < 0) ? 0 : box_y1),
                             (uint16_t)(oct_x + 3 * MENU_CHAR_W), (uint16_t)box_y2, MENU_COL_HI, false);
        }
        for (uint8_t o = 0; o < 4; o++)
        {
            char oct[5];
            memcpy(oct, &value[o * 4], 3);
            oct[3] = '\0';
            draw_big(value_x + (int32_t)o * 4 * MENU_CHAR_W, y,
                     oct, (o == edit_octet) ? 0 : MENU_COL_HI, clip);
            if (o < 3)
            {
                draw_big(value_x + ((int32_t)o * 4 + 3) * MENU_CHAR_W, y, ".", MENU_COL_HI, clip);
            }
        }
        return;
    }

    if (editing)
    {
        int32_t box_y2 = y + 16;
        if (box_y2 > clip)
        {
            box_y2 = clip;
        }
        if (box_y2 >= 0 && (y - 1) <= clip)
        {
            ssd1362_fillRect((uint16_t)(value_x - 2), (uint16_t)((y - 1 < 0) ? 0 : y - 1),
                             (uint16_t)(DISPLAY_WIDTH - 1), (uint16_t)box_y2, MENU_COL_HI, false);
        }
        draw_big(value_x, y, value, 0, clip);
    }
    else
    {
        draw_big(value_x, y, value, MENU_COL_HI, clip);
    }
}

static void draw_items(uint8_t editing, int32_t y_shift, int32_t clip)
{
    const MenuCategory *cat = cur_cat();
    const MenuItem *it = cur_item();

    /* Tab row of every item in the category (windowed when too wide); an
     * active toast takes the row over while it lasts. */
    if (toast[0] != '\0' && (HAL_GetTick() - toast_tick) < MENU_TOAST_MS)
    {
        draw_small_centered(3 + y_shift, toast, MENU_COL_HI, clip);
    }
    else
    {
        const char *labels[16];
        uint8_t n = (cat->count > 16U) ? 16U : cat->count;
        for (uint8_t i = 0; i < n; i++)
        {
            labels[i] = cat->items[i].label;
        }
        draw_tabs(labels, n, item_idx, 3 + y_shift, clip);
    }

    if (editing)
    {
        draw_small(2, 19 + y_shift, (it->vtype == VT_IP && edit_octet < 3U) ? "OK=NEXT" : "OK=SET", MENU_COL_DIM, clip);
    }
    else if (it->kind != MI_CFG)
    {
        draw_small(2, 19 + y_shift, "PRESS OK", MENU_COL_DIM, clip);
    }
    draw_itemValue(it, editing, 15 + y_shift, clip);
}

static void draw_cal_scan(int32_t y_shift, int32_t clip)
{
    draw_header("SCAN CALIBRATION", "< BACK", 1 + y_shift, clip);
    draw_big_centered(9 + y_shift, "OK = START", MENU_COL_HI, clip);
}

static void draw_cal_bp(int32_t y_shift, int32_t clip)
{
    draw_header("BLACK POINT CAL", (cal_phase == CAL_RUNNING) ? NULL : "< BACK", 1 + y_shift, clip);
    const char *msg = (cal_phase == CAL_RUNNING) ? "RUNNING..." :
                      (cal_phase == CAL_DONE)    ? "DONE" :
                      (cal_phase == CAL_FAILED)  ? "FAILED" : "OK = START";
    draw_big_centered(9 + y_shift, msg, MENU_COL_HI, clip);
}

static void draw_confirm_reset(int32_t y_shift, int32_t clip)
{
    draw_header("ERASE ALL SETTINGS?", (cal_phase == CAL_RUNNING) ? NULL : "< BACK", 1 + y_shift, clip);
    const char *msg = (cal_phase == CAL_RUNNING) ? "RESETTING..." :
                      (cal_phase == CAL_DONE)    ? "DONE - REBOOT" :
                      (cal_phase == CAL_FAILED)  ? "FAILED" : "OK = CONFIRM";
    draw_big_centered(9 + y_shift, msg, MENU_COL_HI, clip);
}

static void draw_cal_imu(int32_t y_shift, int32_t clip)
{
    draw_header("IMU CALIBRATION", (cal_phase == CAL_RUNNING) ? "KEEP STILL" : "< BACK", 1 + y_shift, clip);

    char cell[3][12];
    char line[44];

    fmt_imu(cell[0], sizeof(cell[0]), shared_imu.acc[0]);
    fmt_imu(cell[1], sizeof(cell[1]), shared_imu.acc[1]);
    fmt_imu(cell[2], sizeof(cell[2]), shared_imu.acc[2]);
    snprintf(line, sizeof(line), "ACC %s %s %s G", cell[0], cell[1], cell[2]);
    draw_small(2, 10 + y_shift, line, MENU_COL_DIM, clip);

    fmt_imu(cell[0], sizeof(cell[0]), shared_imu.gyro[0]);
    fmt_imu(cell[1], sizeof(cell[1]), shared_imu.gyro[1]);
    fmt_imu(cell[2], sizeof(cell[2]), shared_imu.gyro[2]);
    snprintf(line, sizeof(line), "GYR %s %s %s DPS", cell[0], cell[1], cell[2]);
    draw_small(2, 19 + y_shift, line, MENU_COL_DIM, clip);

    const char *msg = (cal_phase == CAL_RUNNING) ? "CALIBRATING..." :
                      (cal_phase == CAL_DONE)    ? "DONE" :
                      (cal_phase == CAL_FAILED)  ? "FAILED" : "OK = START (KEEP STILL)";
    draw_small_centered(30 + y_shift, msg, MENU_COL_HI, clip);
}

void gui_menu_draw(void)
{
    if (band_h <= 0)
    {
        return;
    }

    /* Band background + sliding bottom edge (same visual language as the
     * host overlay band). */
    if (band_h >= 2)
    {
        ssd1362_fillRect(0, 0, DISPLAY_WIDTH - 1, (uint16_t)(band_h - 2), MENU_COL_BG, false);
    }
    ssd1362_drawHLine(0, (uint16_t)(band_h - 1), DISPLAY_WIDTH, MENU_COL_SEP, false);

    const int32_t y_shift = band_h - menu_fullHeight(draw_state);
    const int32_t clip = band_h - 2;
    if (clip < 0)
    {
        return;
    }

    switch (draw_state)
    {
        case MENU_ROOT:
            draw_root(y_shift, clip);
            break;
        case MENU_ITEMS:
            draw_items(0, y_shift, clip);
            break;
        case MENU_EDIT:
            draw_items(1, y_shift, clip);
            break;
        case MENU_CAL_SCAN:
            draw_cal_scan(y_shift, clip);
            break;
        case MENU_CAL_BP:
            draw_cal_bp(y_shift, clip);
            break;
        case MENU_CAL_IMU:
            draw_cal_imu(y_shift, clip);
            break;
        case MENU_CONFIRM_RESET:
            draw_confirm_reset(y_shift, clip);
            break;
        default:
            break;
    }
}
