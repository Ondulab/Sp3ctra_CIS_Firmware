/**
 ******************************************************************************
 * @file           : file_manager_config.c
 * @brief          : Configuration and persistent data file services.
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

#ifdef CORE_CM7

#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "globals.h"

#include "ff.h"
#include "diskio.h"

#include <stdio.h>

#include "file_manager_config.h"

/* Private define ------------------------------------------------------------*/
#define WORKING_BUFFER_SIZE (2 * _MAX_SS)

/* Private variables ---------------------------------------------------------*/
const struct shared_config DefaultConfig =
{
    /* Volontairement vide : le mot de passe est tire au sort au premier
     * demarrage. Une valeur par defaut identique sur toutes les machines ne
     * protegerait de rien des que la documentation circule. */
    .admin_password = "",
    .admin_password_ack = 0,
    .network_ip = DEFAULT_NETWORK_IP,
    .network_netmask = DEFAULT_NETWORK_NETMASK,
    .network_gw = DEFAULT_NETWORK_GW,
    .network_dest_ip = DEFAULT_NETWORK_DEST_IP,
    .network_link_port = DEFAULT_NETWORK_LINK_PORT,
    .network_udp_port = DEFAULT_NETWORK_CIS_UDP_PORT,
    .stream_when_unbound = DEFAULT_STREAM_WHEN_UNBOUND,
    .cis_print_calibration = DEFAULT_CIS_PRINT_CALIBRATION,
    .cis_dpi = DEFAULT_CIS_DPI,
    .cis_oversampling = DEFAULT_CIS_OVERSAMPLING,
    .cis_handedness = DEFAULT_CIS_HANDEDNESS,
    .imu_gyro_sensitivity = DEFAULT_GYRO_SENSITIVITY,
    .imu_accel_sensitivity = DEFAULT_ACCEL_SENSITIVITY,
    .gui_show_imu = DEFAULT_GUI_SHOW_IMU,
    .gui_invert_cis_image = DEFAULT_GUI_INVERT_CIS_IMAGE,
    .screensaver_timeout_sec = DEFAULT_SCREENSAVER_TIMEOUT_SEC,
    .motion_threshold_acc = DEFAULT_MOTION_THRESHOLD_ACC,
    .motion_threshold_gyro = DEFAULT_MOTION_THRESHOLD_GYRO,
};

FATFS fs;

/* Private function prototypes -----------------------------------------------*/
static fileManager_StatusTypeDef file_parseLine(char* line, volatile struct shared_config* config);
static fileManager_StatusTypeDef print_shared_config(struct shared_config config);

fileManager_StatusTypeDef file_readConfig(const char* filePath, volatile struct shared_config* config)
{
    FIL file;
    FRESULT fr;
    char line[128];

    fr = f_open(&file, filePath, FA_READ);
    if (fr != FR_OK)
    {
        return FILEMANAGER_ERROR;
    }

    while (f_gets(line, sizeof(line), &file))
    {
        (void)file_parseLine(line, config);
    }

    (void)f_close(&file);
    return FILEMANAGER_OK;
}

static fileManager_StatusTypeDef file_parseLine(char* line, volatile struct shared_config* config)
{
    char* token = strtok(line, "=");
    while (token != NULL)
    {
        char* value = strtok(NULL, "\r\n");
        if (value != NULL)
        {
            if (strncmp(token, "NETWORK_IP_ADDR", 15) == 0)
            {
                int index = token[15] - '0';
                if (index >= 0 && index < 4)
                {
                    config->network_ip[index] = (uint8_t)strtoul(value, NULL, 10);
                }
            }
            else if (strncmp(token, "NETWORK_NETMASK_ADDR", 20) == 0)
            {
                int index = token[20] - '0';
                if (index >= 0 && index < 4)
                {
                    config->network_netmask[index] = (uint8_t)strtoul(value, NULL, 10);
                }
            }
            else if (strncmp(token, "NETWORK_GW_ADDR", 15) == 0)
            {
                int index = token[15] - '0';
                if (index >= 0 && index < 4)
                {
                    config->network_gw[index] = (uint8_t)strtoul(value, NULL, 10);
                }
            }
            else if (strncmp(token, "NETWORK_DEST_IP_ADDR", 20) == 0)
            {
                int index = token[20] - '0';
                if (index >= 0 && index < 4)
                {
                    config->network_dest_ip[index] = (uint8_t)strtoul(value, NULL, 10);
                }
            }
            else if (strcmp(token, "NETWORK_UDP_PORT") == 0)
            {
                config->network_udp_port = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "NETWORK_LINK_PORT") == 0)
            {
                config->network_link_port = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "STREAM_WHEN_UNBOUND") == 0)
            {
                config->stream_when_unbound = (strtoul(value, NULL, 10) > 0UL) ? 1U : 0U;
            }
            else if (strcmp(token, "CIS_PRINT_CALIBRATION") == 0)
            {
                config->cis_print_calibration = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_DPI") == 0)
            {
                config->cis_dpi = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_OVERSAMPLING") == 0)
            {
                config->cis_oversampling = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_HANDEDNESS") == 0)
            {
                config->cis_handedness = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "IMU_GYRO_SENSITIVITY") == 0)
            {
                config->imu_gyro_sensitivity = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "IMU_ACCEL_SENSITIVITY") == 0)
            {
                config->imu_accel_sensitivity = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "GUI_SHOW_IMU") == 0)
            {
                config->gui_show_imu = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "GUI_INVERT_CIS_IMAGE") == 0)
            {
                config->gui_invert_cis_image = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "SCREENSAVER_TIMEOUT_SEC") == 0)
            {
                config->screensaver_timeout_sec = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "MOTION_THRESHOLD_ACC") == 0)
            {
                config->motion_threshold_acc = strtof(value, NULL);
            }
            else if (strcmp(token, "MOTION_THRESHOLD_GYRO") == 0)
            {
                config->motion_threshold_gyro = strtof(value, NULL);
            }
            else if (strcmp(token, "ADMIN_PASSWORD") == 0)
            {
                strncpy((char *)config->admin_password, value, ADMIN_PASSWORD_LEN);
                config->admin_password[ADMIN_PASSWORD_LEN] = '\0';
            }
            else if (strcmp(token, "ADMIN_PASSWORD_ACK") == 0)
            {
                config->admin_password_ack = (uint8_t)strtoul(value, NULL, 10);
            }
        }

        token = strtok(NULL, "=");
    }

    return FILEMANAGER_OK;
}

fileManager_StatusTypeDef file_writeConfig(const char* filePath, const volatile struct shared_config* config)
{
    FIL file;
    FRESULT fr;

    fr = f_open(&file, filePath, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        return FILEMANAGER_ERROR;
    }

    f_printf(&file, "NETWORK_IP_ADDR0=%u\n", config->network_ip[0]);
    f_printf(&file, "NETWORK_IP_ADDR1=%u\n", config->network_ip[1]);
    f_printf(&file, "NETWORK_IP_ADDR2=%u\n", config->network_ip[2]);
    f_printf(&file, "NETWORK_IP_ADDR3=%u\n", config->network_ip[3]);
    f_printf(&file, "NETWORK_NETMASK_ADDR0=%u\n", config->network_netmask[0]);
    f_printf(&file, "NETWORK_NETMASK_ADDR1=%u\n", config->network_netmask[1]);
    f_printf(&file, "NETWORK_NETMASK_ADDR2=%u\n", config->network_netmask[2]);
    f_printf(&file, "NETWORK_NETMASK_ADDR3=%u\n", config->network_netmask[3]);
    f_printf(&file, "NETWORK_GW_ADDR0=%u\n", config->network_gw[0]);
    f_printf(&file, "NETWORK_GW_ADDR1=%u\n", config->network_gw[1]);
    f_printf(&file, "NETWORK_GW_ADDR2=%u\n", config->network_gw[2]);
    f_printf(&file, "NETWORK_GW_ADDR3=%u\n", config->network_gw[3]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR0=%u\n", config->network_dest_ip[0]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR1=%u\n", config->network_dest_ip[1]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR2=%u\n", config->network_dest_ip[2]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR3=%u\n", config->network_dest_ip[3]);
    f_printf(&file, "NETWORK_UDP_PORT=%u\n", config->network_udp_port);
    f_printf(&file, "NETWORK_LINK_PORT=%u\n", config->network_link_port);
    f_printf(&file, "STREAM_WHEN_UNBOUND=%u\n", config->stream_when_unbound);
    f_printf(&file, "CIS_PRINT_CALIBRATION=%u\n", config->cis_print_calibration);
    f_printf(&file, "CIS_DPI=%u\n", config->cis_dpi);
    f_printf(&file, "CIS_OVERSAMPLING=%u\n", config->cis_oversampling);
    f_printf(&file, "CIS_HANDEDNESS=%u\n", config->cis_handedness);
    f_printf(&file, "IMU_GYRO_SENSITIVITY=%u\n", config->imu_gyro_sensitivity);
    f_printf(&file, "IMU_ACCEL_SENSITIVITY=%u\n", config->imu_accel_sensitivity);

    f_printf(&file, "GUI_SHOW_IMU=%u\n", config->gui_show_imu);
    f_printf(&file, "GUI_INVERT_CIS_IMAGE=%u\n", config->gui_invert_cis_image);
    f_printf(&file, "SCREENSAVER_TIMEOUT_SEC=%u\n", config->screensaver_timeout_sec);
    f_printf(&file, "ADMIN_PASSWORD=%s\n", config->admin_password);
    f_printf(&file, "ADMIN_PASSWORD_ACK=%u\n", config->admin_password_ack);

    char float_buffer[32];
    sprintf(float_buffer, "MOTION_THRESHOLD_ACC=%.2f\n", config->motion_threshold_acc);
    f_puts(float_buffer, &file);
    sprintf(float_buffer, "MOTION_THRESHOLD_GYRO=%.2f\n", config->motion_threshold_gyro);
    f_puts(float_buffer, &file);

    (void)f_close(&file);
    return FILEMANAGER_OK;
}

static fileManager_StatusTypeDef print_shared_config(struct shared_config config)
{
    printf("=========== CONFIG ============\n");

    printf("Network IP: %u.%u.%u.%u\n",
           config.network_ip[0], config.network_ip[1],
           config.network_ip[2], config.network_ip[3]);

    printf("Network Netmask: %u.%u.%u.%u\n",
           config.network_netmask[0], config.network_netmask[1],
           config.network_netmask[2], config.network_netmask[3]);

    printf("Network Gateway: %u.%u.%u.%u\n",
           config.network_gw[0], config.network_gw[1],
           config.network_gw[2], config.network_gw[3]);

    printf("Network Destination IP: %u.%u.%u.%u\n",
           config.network_dest_ip[0], config.network_dest_ip[1],
           config.network_dest_ip[2], config.network_dest_ip[3]);

    printf("Network UDP Port: %u\n", config.network_udp_port);
    printf("Network Link Port: %u\n", config.network_link_port);
    printf("Stream when unbound: %u\n", config.stream_when_unbound);
    printf("CIS Print Calibration: %u\n", config.cis_print_calibration);
    printf("CIS DPI: %u\n", config.cis_dpi);
    printf("CIS Oversampling: %u\n", config.cis_oversampling);
    printf("CIS Handedness: %u\n", config.cis_handedness);
    printf("===============================\n");

    return FILEMANAGER_OK;
}

fileManager_StatusTypeDef file_factoryReset(void)
{
    printf("- CONFIG FILE TO FACTORY RESET -\n");

    FRESULT fres;

    printf("Unmounting filesystem...\n");
    fres = f_mount(NULL, "0:", 0);
    if (fres != FR_OK)
    {
        printf("Warning: Failed to unmount filesystem (continuing anyway)\n");
    }

    printf("Attempting to format the QSPI flash...\n");

    BYTE work[WORKING_BUFFER_SIZE];

    fres = f_mkfs("0:", FM_ANY, 0, work, WORKING_BUFFER_SIZE);
    if (fres != FR_OK)
    {
        printf("Failed to format the QSPI flash.\n");
        return FILEMANAGER_ERROR;
    }

    printf("Format successful\n");

    printf("Remounting filesystem...\n");
    fres = f_mount(&fs, "0:", 1);
    if (fres != FR_OK)
    {
        printf("Warning: Failed to remount filesystem after format\n");
        return FILEMANAGER_ERROR;
    }

    printf("Filesystem remounted successfully\n");

    printf("Loading default configuration...\n");
    shared_config = DefaultConfig;

    printf("Writing default configuration file...\n");
    if (file_writeConfig(CONFIG_FILE_PATH, &shared_config) != FILEMANAGER_OK)
    {
        printf("Failed to write default configuration file\n");
        return FILEMANAGER_ERROR;
    }

    printf("Default configuration written successfully\n");
    (void)print_shared_config(shared_config);

    return FILEMANAGER_OK;
}

fileManager_StatusTypeDef file_initConfig(volatile struct shared_config* config)
{
    FRESULT fres;
    fileManager_StatusTypeDef rv = FILEMANAGER_ERROR;

    // Always start from DefaultConfig so that older CONFIG.TXT files missing
    // newer fields (e.g. NETWORK_LINK_PORT / STREAM_WHEN_UNBOUND) still get sane
    // defaults.
    //
    // Without this, missing keys would leave fields uninitialized (typically 0)
    // which can silently change behavior after a firmware update.
    *config = DefaultConfig;

    fres = f_mount(&fs, "0:", 1);
    if (fres != FR_OK)
    {
        printf("FS mount ERROR\n");
        printf("Attempting to format the QSPI flash...\n");

        BYTE work[WORKING_BUFFER_SIZE];

        fres = f_mkfs("0:", FM_ANY, 0, work, WORKING_BUFFER_SIZE);
        if (fres != FR_OK)
        {
            printf("Failed to format the QSPI flash.\n");
            return rv;
        }

        fres = f_mount(&fs, "0:", 1);
        if (fres != FR_OK)
        {
            printf("Failed to mount the filesystem even after formatting.\n");
            return rv;
        }

        printf("FS mount SUCCESS after formatting.\n");
        rv = FILEMANAGER_OK;
    }
    else
    {
        rv = FILEMANAGER_OK;
    }

    if (file_readConfig(CONFIG_FILE_PATH, config) != 0)
    {
        printf("Failed to read configuration file\n");

        *config = DefaultConfig;

        if (file_writeConfig(CONFIG_FILE_PATH, config) == 0)
        {
            printf("Write configuration SUCCESS\n");

            if (file_readConfig(CONFIG_FILE_PATH, config) == 0)
            {
                printf("Configuration verified and loaded\n");
                (void)print_shared_config(*config);
            }
            else
            {
                printf("Warning: Failed to verify written configuration\n");
                (void)print_shared_config(*config);
            }
        }
        else
        {
            printf("Failed to write configuration file\n");
            return FILEMANAGER_ERROR;
        }
    }
    else
    {
        printf("Read configuration SUCCESS\n");
        (void)print_shared_config(*config);
    }

    // Post-load validation: a zero port means "use default" (older CONFIG.TXT
    // files, or a value cleared by hand).
    if (config->network_link_port == 0U) {
        config->network_link_port = DEFAULT_NETWORK_LINK_PORT;
    }
    if (config->network_udp_port == 0U) {
        config->network_udp_port = DEFAULT_NETWORK_CIS_UDP_PORT;
    }
    if (config->stream_when_unbound > 1U) {
        config->stream_when_unbound = DEFAULT_STREAM_WHEN_UNBOUND;
    }

    return rv;
}

fileManager_StatusTypeDef file_writeCisCals(const char* filePath, const struct cisCals* data)
{
    FIL file;
    UINT bw;
    FRESULT fr;

    fr = f_open(&file, filePath, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        printf("Failed to create calibration file: %s\n", filePath);
        Error_Handler();
        return FILEMANAGER_ERROR;
    }

    fr = f_write(&file, data, sizeof(cisCals), &bw);
    if (fr != FR_OK || bw != sizeof(cisCals))
    {
        printf("Failed to write calibration file\n");
        (void)f_close(&file);
        return FILEMANAGER_ERROR;
    }

    (void)f_close(&file);
    return FILEMANAGER_OK;
}

fileManager_StatusTypeDef file_readCisCals(const char* filePath, struct cisCals* data)
{
    FIL file;
    UINT br;
    FRESULT fr;

    fr = f_open(&file, filePath, FA_READ);
    if (fr != FR_OK)
    {
        return FILEMANAGER_ERROR;
    }

    fr = f_read(&file, data, sizeof(cisCals), &br);
    if (fr != FR_OK || br != sizeof(cisCals))
    {
        (void)f_close(&file);
        return FILEMANAGER_ERROR;
    }

    (void)f_close(&file);
    return FILEMANAGER_OK;
}

fileManager_StatusTypeDef file_writeImuCals(const char* filePath, const struct imuCals* data)
{
    FIL file;
    UINT bw;
    FRESULT fr;

    fr = f_open(&file, filePath, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        printf("Failed to create IMU calibration file: %s\n", filePath);
        return FILEMANAGER_ERROR;
    }

    fr = f_write(&file, data, sizeof(struct imuCals), &bw);
    if (fr != FR_OK || bw != sizeof(struct imuCals))
    {
        printf("Failed to write IMU calibration file\n");
        (void)f_close(&file);
        return FILEMANAGER_ERROR;
    }

    (void)f_close(&file);
    return FILEMANAGER_OK;
}

fileManager_StatusTypeDef file_readImuCals(const char* filePath, struct imuCals* data)
{
    FIL file;
    UINT br;
    FRESULT fr;

    fr = f_open(&file, filePath, FA_READ);
    if (fr != FR_OK)
    {
        return FILEMANAGER_ERROR;
    }

    fr = f_read(&file, data, sizeof(struct imuCals), &br);
    if (fr != FR_OK || br != sizeof(struct imuCals))
    {
        (void)f_close(&file);
        return FILEMANAGER_ERROR;
    }

    (void)f_close(&file);
    return FILEMANAGER_OK;
}

#endif /* CORE_CM7 */
