/**
 ******************************************************************************
 * @file           : file_manager.c
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
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "shared.h"
#include "stdio.h"

#include "ff.h" // FATFS include
#include "diskio.h" // DiskIO include

#include "file_manager.h"

const struct shared_config DefaultConfig =
{
    .ui_button_delay = 500,
    .network_ip = {192, 168, 0, 10},
    .network_netmask = {255, 255, 255, 0},
    .network_gw = {0, 0, 0, 0},
    .network_dest_ip = {192, 168, 0, 255},
    .network_udp_port = 55151,
	.network_broadcast = 1,
    .cis_print_calibration = 0,
    .cis_raw = 0,
    .cis_dpi = 400,
    .cis_monochrome = 0,
    .cis_max_line_freq = 800,
    .cis_clk_freq = 3125000,
    .cis_oversampling = 8,
    .cis_handedness = 1
};

#define WORKING_BUFFER_SIZE (2 * _MAX_SS)

FATFS fs;   // Filesystem object

int file_readConfig(const char* filePath, struct shared_config* config)
{
    FIL file;
    FRESULT fr;
    char line[128];

    fr = f_open(&file, filePath, FA_READ);
    if (fr != FR_OK)
    {
        return -1;
    }

    while (f_gets(line, sizeof(line), &file))
    {
        file_parseLine(line, config);
    }

    f_close(&file);
    return 0;
}

void file_parseLine(char* line, struct shared_config* config)
{
    char* token = strtok(line, "=");
    while (token != NULL)
    {
        char* value = strtok(NULL, "\r\n");
        if (value != NULL)
        {
            if (strcmp(token, "UI_BUTTON_DELAY") == 0)
            {
                config->ui_button_delay = strtoul(value, NULL, 10);
            }
            else if (strncmp(token, "NETWORK_IP_ADDR", 15) == 0)
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
            else if (strcmp(token, "NETWORK_BROADCAST") == 0)
            {
                config->network_broadcast = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_PRINT_CALIBRATION") == 0)
            {
                config->cis_print_calibration = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_RAW") == 0)
            {
                config->cis_raw = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_DPI") == 0)
            {
                config->cis_dpi = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_MONOCHROME") == 0)
            {
                config->cis_monochrome = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_MAX_LINE_FREQ") == 0)
            {
                config->cis_max_line_freq = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_CLK_FREQ") == 0)
            {
                config->cis_clk_freq = strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_OVERSAMPLING") == 0)
            {
                config->cis_oversampling = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_HANDEDNESS") == 0)
            {
                config->cis_handedness = (uint8_t)strtoul(value, NULL, 10);
            }
        }
        token = strtok(NULL, "=");
    }
}

int file_writeConfig(const char* filePath, const struct shared_config* config)
{
    FIL file;
    FRESULT fr;
    fr = f_open(&file, filePath, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        return -1;
    }

    f_printf(&file, "UI_BUTTON_DELAY=%lu\n", config->ui_button_delay);
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
    f_printf(&file, "NETWORK_DEST_IP_ADDR=%u\n", config->network_dest_ip[0]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR=%u\n", config->network_dest_ip[1]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR=%u\n", config->network_dest_ip[2]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR=%u\n", config->network_dest_ip[3]);
    f_printf(&file, "NETWORK_UDP_PORT=%u\n", config->network_udp_port);
    f_printf(&file, "NETWORK_BROADCAST=%u\n", config->network_broadcast);
    f_printf(&file, "CIS_PRINT_CALIBRATION=%u\n", config->cis_print_calibration);
    f_printf(&file, "CIS_RAW=%u\n", config->cis_raw);
    f_printf(&file, "CIS_DPI=%u\n", config->cis_dpi);
    f_printf(&file, "CIS_MONOCHROME=%u\n", config->cis_monochrome);
    f_printf(&file, "CIS_MAX_LINE_FREQ=%u\n", config->cis_max_line_freq);
    f_printf(&file, "CIS_CLK_FREQ=%lu\n", config->cis_clk_freq);
    f_printf(&file, "CIS_OVERSAMPLING=%u\n", config->cis_oversampling);
    f_printf(&file, "CIS_HANDEDNESS=%u\n", config->cis_handedness);

    f_close(&file);
    return 0;
}

void file_initConfig(struct shared_config* config)
{
	printf("- CONFIG FILE INITIALIZATIONS -\n");
	                                          //

	FRESULT fres; // Variable to store the result of FATFS operations

	// Attempt to mount the file system
	fres = f_mount(&fs, "0:", 1); // 1 to mount immediately
	if (fres != FR_OK)
	{
	    printf("FS mount ERROR\n");

	    // If mounting fails, try to format the QSPI flash
	    printf("Attempting to format the QSPI flash...\n");

	    BYTE work[WORKING_BUFFER_SIZE]; // Static allocation to simplify

	    fres = f_mkfs("0:", FM_ANY, 0, work, WORKING_BUFFER_SIZE);
	    if (fres != FR_OK)
	    {
	        printf("Failed to format the QSPI flash.\n");
	    }

	    // Try to mount the file system again after formatting
	    fres = f_mount(&fs, "0:", 1);
	    if (fres != FR_OK)
	    {
	        printf("Failed to mount the filesystem even after formatting.\n");
	    } else
	    {
	        printf("FS mount SUCCESS after formatting.\n");
	    }
	}
	else
	{
	    printf("FS mount SUCCESS\n");
	}

    if (file_readConfig(CONFIG_FILE_PATH, config) != 0)
    {
        printf("Failed to read configuration file\n");
        *config = DefaultConfig;
        if (file_writeConfig(CONFIG_FILE_PATH, config) == 0)
        {
        	 printf("Write configuration SUCCESS\n");
        }
        else
        {
        	printf("Failed to write configuration file\n");
        	Error_Handler();
        }
    }
    else
    {
        printf("Read configuration SUCCESS\n");
    }
}

int file_writeCisCals(const char* filePath, const struct cisCals* data)
{
    FIL file;
    UINT bw;
    FRESULT fr;

    // Open the file in write mode
    fr = f_open(&file, filePath, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
    	printf("Failed to create calibration file\n");
    	Error_Handler();
        return -1;
    }

    // Write data to the file
    fr = f_write(&file, data, sizeof(cisCals), &bw);
    if (fr != FR_OK || bw != sizeof(cisCals))
    {
    	printf("Failed to write calibration file\n");
        f_close(&file);
        return -1;
    }

    // Close the file
    f_close(&file);
    return 0;
}

int file_readCisCals(const char* filePath, struct cisCals* data)
{
    FIL file;
    UINT br;
    FRESULT fr;

    // Open the file in read mode
    fr = f_open(&file, filePath, FA_READ);
    if (fr != FR_OK)
    {
        return -1;
    }

    // Read data from the file
    fr = f_read(&file, data, sizeof(cisCals), &br);
    if (fr != FR_OK || br != sizeof(cisCals))
    {
        f_close(&file);
        return -1;
    }

    // Close the file
    f_close(&file);
    return 0;
}
