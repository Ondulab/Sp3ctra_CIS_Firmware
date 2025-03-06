/**
 ******************************************************************************
 * @file           : httpserver.c
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

/* Includes ------------------------------------------------------------------*/
#include "globals.h"
#include "config.h"
#include "boot_config.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"

#include "string.h"
#include "stdio.h"
#include "stdbool.h"

#include "ff.h" // FATFS include
#include "diskio.h" // DiskIO include

#include "file_manager.h"
#include "cis.h"

#include "stm32_flash.h"

#include "http_server.h"

TaskHandle_t http_ThreadHandle = NULL;

/*!
 * @value(StatusCode_NONE) No status code
 * @value(StatusCode_COMPLETED) Response to a completed API call or the update has completed in case of status poll
 * @value(StatusCode_INVALID_ARG) Response to an invalid API argument
 * @value(StatusCode_INVALID_IMAGE) Reponse to early header verification failure
 * @value(StatusCode_IMAGE_TOO_LARGE) Fail if provided length is too large, or embedded image length is too large
 * @value(StatusCode_IMAGE_TOO_SMALL) Fail if embedded image length is smaller than the minimum data length
 * @value(StatusCode_SECTION_NOT_AVAILABLE) Non-volatile section corresponding to the supplied ImageType cannot be found
 * @value(StatusCode_SECTION_ERASE_FAILURE) Error preparing (erasing) update target section
 * @value(StatusCode_SECTION_WRITE_FAILURE) Error writing data to update target section
 * @value(StatusCode_IMAGE_VERSION_FAILURE) Image version embedded in header is less than version of currently running image and update rejected
 * @value(StatusCode_INVALID_SECTION_KEY) The supplied firmware update image has a different section key than what is associated with the application currently running on the device
 * @value(StatusCode_IMAGE_VERIFY_FAILURE) Response after the completely written firmware image verification has failed
 * @value(StatusCode_INVALID_STATE) Response to an API called in an invalid state, e.g. supplying more data when waiting for reboot
 * @value(StatusCode_INVALID_ORDER) Supplied data with incorrect order for the current state
 * @value(StatusCode_TOO_FEW_BYTES) Supplied first data packet with less than minimum required bytes
 * @value(StatusCode_PARSER_ERROR) Update container format parser has encountered an unrecoverable error in the byte stream
 * @value(StatusCode_DECRYPTION_ERROR) Update container format stream decryption has failed
 * @value(StatusCode_INSTALL_ERROR) Error finalizing the newly written firmware in download slot (automatically or through FW_UPDATE_InstallAtNextReset)
 * @value(StatusCode_FLASH_ERROR) Flash (possibly external) initialization error
 * @value(StatusCode_FLASH_SEGMENT_ERROR) Error initializing the SEGMENT read layer for external flash MultiSegment feature
 * @value(StatusCode_FLASH_CIPHER_ERROR) Error initializing the CIPHER write layer for external flash MultiSegment feature
 */
typedef enum
{
	FW_UPDATE_StatusCode_NONE,
	FW_UPDATE_StatusCode_COMPLETED,
	FW_UPDATE_StatusCode_INVALID_ARG,
	FW_UPDATE_StatusCode_INVALID_PATCH_IMAGE,
	FW_UPDATE_StatusCode_INVALID_SOURCE_IMAGE,
	FW_UPDATE_StatusCode_INVALID_TARGET_IMAGE,
	FW_UPDATE_StatusCode_INVALID_PATCH_TAG,
	FW_UPDATE_StatusCode_IMAGE_TOO_LARGE,
	FW_UPDATE_StatusCode_IMAGE_TOO_SMALL,
	FW_UPDATE_StatusCode_SECTION_NOT_AVAILABLE,
	FW_UPDATE_StatusCode_SECTION_ERASE_FAILURE,
	FW_UPDATE_StatusCode_SECTION_WRITE_FAILURE,
	FW_UPDATE_StatusCode_IMAGE_VERSION_FAILURE,
	FW_UPDATE_StatusCode_INVALID_SECTION_KEY,
	FW_UPDATE_StatusCode_IMAGE_VERIFY_TAG_FAILURE,
	FW_UPDATE_StatusCode_IMAGE_VERIFY_ALG_FAILURE,
	FW_UPDATE_StatusCode_IMAGE_DECRYPT_FAILURE,
	FW_UPDATE_StatusCode_INVALID_STATE,
	FW_UPDATE_StatusCode_INVALID_ORDER,
	FW_UPDATE_StatusCode_TOO_FEW_BYTES,
	FW_UPDATE_StatusCode_PARSER_ERROR,
	FW_UPDATE_StatusCode_DECRYPTION_ERROR,
	FW_UPDATE_StatusCode_INSTALL_ERROR,
	FW_UPDATE_StatusCode_FLASH_ERROR,
	FW_UPDATE_StatusCode_FLASH_SEGMENT_ERROR,
	FW_UPDATE_StatusCode_FLASH_CIPHER_ERROR,
	FW_UPDATE_StatusCode__MAX__
}  FW_UPDATE_StatusCode;

typedef enum {
	FWUPDATE_STATE_HEADER,
	FWUPDATE_STATE_DOWNLOAD_START,
	FWUPDATE_STATE_DOWNLOAD_STREAM,
} fwupdate_state_t;

typedef enum
{
	FW_UPDATE_Stage_IDLE,
	FW_UPDATE_Stage_UPDATE,
	FW_UPDATE_Stage_VERIFIED
}  FW_UPDATE_Stage;

/**
 * @brief  Secure Engine Error definition
 */
typedef enum
{
	FU_ERROR = 0U,
	FU_SUCCESS = !FU_ERROR
} FU_ErrorStatus;

typedef struct {

	fwupdate_state_t state;

	int content_length;
	int file_length;
	int accum_length;

	bool completed;
	bool error;
	FW_UPDATE_StatusCode code;
	FW_UPDATE_Stage stage;

	/* Data stream accumulation buffer file update */
	uint32_t has_been_initialized;
	uint8_t accum_buf[8];
	uint8_t accum_buf_len;

} fwupdate_t;

static fwupdate_t fwupdate;

#define CONTENT_LENGTH_TAG      "Content-Length:"
#define DOWNLOAD_STREAM_TAG      "application/octet-stream\r\n\r\n"
#define DOWNLOAD_STREAM_TAG_2      "application/macbinary\r\n\r\n"
#define EMPTY_LINE_TAG          "\r\n\r\n"

#define FWUPDATE_STATUS_ERROR          -1
#define FWUPDATE_STATUS_NONE            0
#define FWUPDATE_STATUS_INPROGRESS      1
#define FWUPDATE_STATUS_DONE            2

#define FW_UPDATE_Stage_NOT_STARTED 0
#define FW_UPDATE_Stage_IN_PROGRESS 1
#define FW_UPDATE_Stage_VERIFIED 2

static FIL file;

#define FW_UPDATE_RebootDelay_NEXT            ((uint32_t)1)

#ifdef HTTP_SERVER_DEBUG
static const char* fwupdate_state_str(fwupdate_state_t state)
{
	switch (state) {
	case FWUPDATE_STATE_HEADER: return "HEADER";
	case FWUPDATE_STATE_DOWNLOAD_START: return "DOWNLOAD_START";
	case FWUPDATE_STATE_DOWNLOAD_STREAM: return "DOWNLOAD_STREAM";
	default:
		return "<unknown>";
	}

}
#endif

/*!
 * High-level patching engine image types.
 *
 * These are the types of firmware images that may be presented to the patching engine.
 */
typedef enum
{
	FW_UPDATE_ImageType_NONE = 0,
	FW_UPDATE_ImageType_APP
}  FW_UPDATE_ImageType;

void delete_old_firmware(const char *latest_firmware)
{
    DIR dir;
    FILINFO fno;
    FRESULT res;

    // Open the FW_PATH directory
    res = f_opendir(&dir, FW_PATH);
    if (res != FR_OK)
    {
        printf("Error opening firmware directory: %d\n", res);
        return;
    }

    while (1)
    {
        // Read the next entry
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == '\0')
        {
            // Either a read error or end of directory
            break;
        }

        // (Optional) If you want to explicitly ignore subdirectories,
        // you can do it like this:
        // if (fno.fattrib & AM_DIR)
        // {
        //     // It's a directory, ignore it
        //     continue;
        // }

        // Compare the filename with the firmware to keep
        if (strcmp(fno.fname, latest_firmware) != 0)
        {
            // Construct the full path: "FW_PATH/fno.fname"
            char filepath[FILE_NAME_MAX_LENGTH];
            size_t needed_length = strlen(FW_PATH) + 1 /* slash or backslash */ + strlen(fno.fname) + 1 /* '\0' */;

            // Check if we exceed the buffer size
            if (needed_length > sizeof(filepath))
            {
                printf("Path too long, unable to delete %s\n", fno.fname);
                continue;
            }

            // Assume FW_PATH is defined and fno.fname is provided by FATFS.
            // We calculate the available space for fno.fname.
            size_t available_space = sizeof(filepath) - strlen(FW_PATH) - 1; // -1 for the '/'

            // Using %.*s to limit the number of characters copied from fno.fname.
            snprintf(filepath, sizeof(filepath), "%s/%.*s", FW_PATH, (int)available_space, fno.fname);

            // Delete the file
            res = f_unlink(filepath);
            if (res == FR_OK)
            {
                printf("File deleted: %s\n", filepath);
            }
            else
            {
                printf("Error deleting %s: %d\n", filepath, res);
            }
        }
    }
    // Close the directory
    f_closedir(&dir);
}

#define HEADER_BUF_SIZE 2048

static int fwupdate_octetstream_state_machine(struct netconn *conn, char *buf, u16_t buflen)
{
    // Buffer statique pour accumuler l'en‐tête sur plusieurs appels
    static char header_accum[HEADER_BUF_SIZE];
    static int header_accum_len = 0;

    // Variables statiques pour conserver l'état une fois le header reçu
    static int header_parsed = 0;   // 0 : header pas encore entièrement reçu, 1 : header traité
    static int header_length = 0;   // Taille de l'en‐tête complet (jusqu'à "\r\n\r\n")
    static int content_length = 0;  // Valeur de Content-Length (taille du fichier)
    static int file_data_length = 0;// Taille réelle du fichier (ici = Content-Length)
    static int accum_length = 0;    // Nombre d'octets déjà écrits dans le fichier
    static char file_name[FILE_NAME_MAX_LENGTH] = {0};
    static char full_file_path[FILE_NAME_MAX_LENGTH] = {0};
    int ret = FWUPDATE_STATUS_NONE;

    if (!header_parsed)
    {
        // Accumulez les données reçues dans header_accum
        if (header_accum_len + buflen > HEADER_BUF_SIZE)
        {
            // Buffer insuffisant
            return FWUPDATE_STATUS_ERROR;
        }
        memcpy(header_accum + header_accum_len, buf, buflen);
        header_accum_len += buflen;

        // Recherche de la fin de l'en‐tête
        char *header_end = strstr(header_accum, "\r\n\r\n");
        if (header_end == NULL)
        {
            // En‐tête pas encore complet, attendre plus de données
            return FWUPDATE_STATUS_INPROGRESS;
        }
        // L'en‐tête est complet : on détermine sa longueur
        header_length = (int)(header_end - header_accum) + 4;

        // Extraction du Content-Length
        char *cl_ptr = strstr(header_accum, "Content-Length:");
        if (cl_ptr == NULL)
        {
            return FWUPDATE_STATUS_ERROR;
        }
        cl_ptr += strlen("Content-Length:");
        content_length = atoi(cl_ptr);
        // Dans ce mode, le client doit envoyer uniquement le fichier en brut.
        file_data_length = content_length;

        // Utilisation d'un nom de fichier fixe (vous pouvez aussi le passer en paramètre)
        strcpy(file_name, "firmware.bin");
        snprintf(full_file_path, sizeof(full_file_path), "%s/%s", FW_PATH, file_name);

        // Supprime les anciens firmwares (tout fichier différent de file_name)
        delete_old_firmware(file_name);

        // Ouvre le fichier en écriture
        FRESULT fr = f_open(&file, full_file_path, FA_WRITE | FA_CREATE_ALWAYS);
        if (fr != FR_OK)
        {
            return FWUPDATE_STATUS_ERROR;
        }

        accum_length = 0;
        header_parsed = 1;

        // Traite la partie utile (après l'en‐tête) présente dans header_accum
        int leftover = header_accum_len - header_length;
        if (leftover > 0)
        {
            UINT bytes_written = 0;
            int to_write = leftover;
            if (to_write > file_data_length)
                to_write = file_data_length;
            fr = f_write(&file, header_accum + header_length, (UINT)to_write, &bytes_written);
            if (fr != FR_OK || bytes_written != (UINT)to_write)
            {
                f_close(&file);
                header_parsed = 0;
                header_accum_len = 0;
                return FWUPDATE_STATUS_ERROR;
            }
            accum_length += to_write;
        }
        // On réinitialise le buffer d'en‐tête pour que les données suivantes soient traitées normalement
        header_accum_len = 0;

        if (accum_length >= file_data_length)
        {
            f_close(&file);
            header_parsed = 0;
            ret = FWUPDATE_STATUS_DONE;
        }
        else
        {
            ret = FWUPDATE_STATUS_INPROGRESS;
        }
    }
    else
    {
        // Header déjà traité, on reçoit uniquement des données du fichier
        int remaining = file_data_length - accum_length;
        int to_write = buflen;
        if (to_write > remaining)
            to_write = remaining;
        UINT bytes_written = 0;
        FRESULT fr = f_write(&file, buf, (UINT)to_write, &bytes_written);
        if (fr != FR_OK || bytes_written != (UINT)to_write)
        {
            f_close(&file);
            header_parsed = 0;
            return FWUPDATE_STATUS_ERROR;
        }
        accum_length += to_write;
        if (accum_length >= file_data_length)
        {
            f_close(&file);
            header_parsed = 0;
            ret = FWUPDATE_STATUS_DONE;
        }
        else
        {
            ret = FWUPDATE_STATUS_INPROGRESS;
        }
    }
    return ret;
}

static int fwupdate_multipart_state_machine(struct netconn *conn, char *buf, u16_t buflen)
{
    int ret = FWUPDATE_STATUS_NONE;
    char *buf_start = buf;
    char *buf_end = buf + buflen; // Points to byte AFTER end of buffer!
    char file_name[FILE_NAME_MAX_LENGTH] = {0};  // Buffer to store the file name.
    char full_file_path[FILE_NAME_MAX_LENGTH] = {0};

    char response[100];

    DIR dir;
    FRESULT fres = f_opendir(&dir, FW_PATH);
    if (fres != FR_OK)
    {
        printf("@ fwupdate - WARNING: Firmware directory missing, creating it...\n");
        fres = f_mkdir(FW_PATH);
        if (fres != FR_OK)
        {
            printf("@ fwupdate - ERROR: Failed to create /firmware/ directory! (FR=%d)\n", fres);
            return FWUPDATE_STATUS_ERROR;
        }
    }
    else
    {
        f_closedir(&dir);
    }

    while (buf && buf < buf_end)
    {
#ifdef HTTP_SERVER_DEBUG
        printf("@ fwupdate buf_start=%p, buf=%p buf_end=%p state=%s\n",
               buf_start, buf, buf_end, fwupdate_state_str(fwupdate.state));
#endif

        switch (fwupdate.state)
        {
            case FWUPDATE_STATE_HEADER:
            {
                if ((buflen >= 12) && (strncmp(buf, "POST /upload", 12) == 0))
                {
                    ret = FWUPDATE_STATUS_ERROR; // Error until we go to in progress
                    printf("@ fwupdate -     Scanning HEADER\n");
                    buf = strstr(buf, CONTENT_LENGTH_TAG);
                    if (buf)
                    {
                        buf += strlen(CONTENT_LENGTH_TAG);
                        fwupdate.content_length = atoi(buf);
                        buf = strstr(buf, EMPTY_LINE_TAG);
                        if (buf)
                        {
                            buf += strlen(EMPTY_LINE_TAG);
                            buf_start = buf;
                            printf("@ fwupdate - Have content len=%d\n", fwupdate.content_length);
                        }
                        else
                        {
                            buf = 0;
                            printf("@ fwupdate - Error extracting empty line tag\n");
                            ret = FWUPDATE_STATUS_ERROR;
                        }

                        buf = strstr(buf, "Content-Disposition:");
                        if (buf)
                        {
                            int extracted = sscanf(buf,
                                                   "Content-Disposition: form-data; name=\"firmware\"; filename=\"%255[^\"]\"",
                                                   file_name);
                            if (extracted == 1)
                            {
                                // Calculate the available space for file_name after FW_PATH and '/'
                                size_t available_space = sizeof(full_file_path) - strlen(FW_PATH) - 2; // -1 for '/' and -1 for null terminator

                                if (available_space > 0)
                                {
                                    snprintf(full_file_path, sizeof(full_file_path), "%s/%.*s", FW_PATH, (int) available_space, file_name);
                                }
                                else
                                {
                                    // Handle error: not enough space to append file_name.
                                    printf("@ fwupdate - Not enough space to append file_name\n");
                                    ret = FWUPDATE_STATUS_ERROR;
                                }

                                fwupdate.state = FWUPDATE_STATE_DOWNLOAD_START;
                                ret = FWUPDATE_STATUS_INPROGRESS;
                            }
                            else
                            {
                                printf("@ fwupdate - Error extracting file name\n");
                                ret = FWUPDATE_STATUS_ERROR;
                            }
                        }
                        else
                        {
                            printf("@ fwupdate - No Content-Disposition field found\n");
                            ret = FWUPDATE_STATUS_ERROR;
                        }
                    }
                }
                else
                {
                    buf = 0;
                }
                break;
            }

            case FWUPDATE_STATE_DOWNLOAD_START:
            {
                // Locate the end of HTTP headers
                char *header_end = strstr(buf, "\r\n\r\n");
                if (header_end != NULL)
                {
                    header_end += 4;  // Skip past "\r\n\r\n"
                    size_t header_length = header_end - buf_start;
                    // Calculate the expected file data length based on Content-Length
                    fwupdate.file_length = fwupdate.content_length - header_length;
#ifdef HTTP_SERVER_DEBUG
                    printf("@ fwupdate: Content-Length = %d, header length = %lu, file length = %d\n",
                           fwupdate.content_length, (unsigned long)header_length, fwupdate.file_length);
#endif
                    // Open the file for writing (initial creation)
                    FRESULT fr = f_open(&file, full_file_path, FA_WRITE | FA_CREATE_ALWAYS);
                    if (fr != FR_OK)
                    {
                        fwupdate.code = fr;
                        ret = FWUPDATE_STATUS_ERROR;
                        break;
                    }
                    f_close(&file);
                    // Move buffer pointer to the start of file content
                    buf = header_end;
                    // Transition to data download state
                    fwupdate.state = FWUPDATE_STATE_DOWNLOAD_STREAM;
                    fwupdate.accum_length = 0;
                    ret = FWUPDATE_STATUS_INPROGRESS;
                }
                else
                {
                    printf("@ fwupdate - End of headers not found.\n");
                    ret = FWUPDATE_STATUS_ERROR;
                }
                break;
            }

            case FWUPDATE_STATE_DOWNLOAD_STREAM:
            {
                if ((buf_end - buf) > 0)
                {
                    UINT bytes_written = 0;
                    uint32_t data_len = (uint32_t)(buf_end - buf);
                    ret = FWUPDATE_STATUS_INPROGRESS;

                    if (!fwupdate.has_been_initialized)
                    {
                        FRESULT fr = f_open(&file, full_file_path, FA_WRITE | FA_CREATE_ALWAYS);
                        if (fr != FR_OK)
                        {
                            fwupdate.code = fr;
                            ret = FWUPDATE_STATUS_ERROR;
                            break;
                        }
                        fwupdate.has_been_initialized = 1;
                        fwupdate.stage = FW_UPDATE_Stage_IN_PROGRESS;
                    }

                    // Write the received data to the file
                    FRESULT fr = f_write(&file, buf, data_len, &bytes_written);
                    if ((fr != FR_OK) || (bytes_written != data_len))
                    {
                        fwupdate.code = fr;
                        f_close(&file);
                        ret = FWUPDATE_STATUS_ERROR;
                        break;
                    }

                    // Update the accumulator with the number of bytes written
                    fwupdate.accum_length += data_len;
#ifdef HTTP_SERVER_DEBUG
                    printf("@ fwupdate: Accumulated %d bytes\n", (int)fwupdate.accum_length);
#endif

                    // Check if we have received all expected file data
                    if (fwupdate.accum_length >= fwupdate.file_length)
                    {
                        // Optionally, check for a boundary marker at the end of the buffer
                        const char *boundary_marker = "\r\n-----------------------------";
                        char *boundary_ptr = strstr(buf, boundary_marker);
                        if (boundary_ptr != NULL)
                        {
                            // Calculate extra bytes (boundary) present in the last buffer segment
                            size_t extra_bytes = (buf_end - boundary_ptr);
                            fwupdate.file_length -= extra_bytes;
                            printf("@ fwupdate: Adjusted file length by removing %u boundary bytes, new file length = %d\n",
                                   (unsigned int)extra_bytes, fwupdate.file_length);

                            // Truncate the file to the correct length
                            f_lseek(&file, fwupdate.file_length);
                            f_truncate(&file);
                        }

                        // Finalize the update (par exemple, suppression des anciens firmwares)
                        delete_old_firmware(file_name);
                        f_close(&file);
                        fwupdate.has_been_initialized = 0;
                        fwupdate.stage = FW_UPDATE_Stage_VERIFIED;
                    }

                    buf = 0;

                    if (ret == FWUPDATE_STATUS_INPROGRESS)
                    {
                        // Si le transfert est terminé ou validé, envoyer la réponse HTTP
                        if ((fwupdate.accum_length >= fwupdate.file_length) ||
                            (fwupdate.stage == FW_UPDATE_Stage_VERIFIED))
                        {
                            ret = FWUPDATE_STATUS_DONE;
                            int len = sprintf(response,
                                              "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nUpdate Successful.\r\n");
                            netconn_write(conn, response, len, NETCONN_COPY);
                            // Réinitialiser l'état pour une nouvelle mise à jour
                            fwupdate.state = FWUPDATE_STATE_HEADER;
                        }
                    }
                }
                break;
            }
        }
    }

    if (ret == FWUPDATE_STATUS_ERROR)
    {
        fwupdate.state = FWUPDATE_STATE_HEADER;
    }

    return ret;
}

static void http_server(struct netconn *conn)
{
	struct netbuf *inbuf;
	err_t recv_err;
	char* buf;
	u16_t buflen;
	struct fs_file file;
	/* Normal GET requests are expected to be closed by us after sending the response. */
	bool close = true;
	bool reboot = false;

#ifdef HTTP_SERVER_DEBUG
	printf("===== http_server_serve recv\n");
#endif

    if (conn == NULL)
    {
        printf("Error: Null connection passed to http_server.\n");
        return;
    }

	/* Read the data from the port, blocking if nothing yet there.
	   We assume the request (the part we care about) is in one netbuf */
	while ((recv_err = netconn_recv(conn, &inbuf)) == ERR_OK)
	{
		if (netconn_err(conn) == ERR_OK)
		{
			do {
				netbuf_data(inbuf, (void**)&buf, &buflen);
#ifdef HTTP_SERVER_DEBUG
				printf("# Process buffer: %p %d bytes\n", buf, buflen);
#endif

				/* Is this an HTTP GET command? (only check the first 5 chars, since
	            there are other formats for GET, and we're keeping it very simple )*/
				if ((buflen >= 5) && (strncmp(buf, "GET /", 5) == 0))
				{

					/* Check for various paths and handle GET requests */
					if (strncmp((char const *)buf, "GET /config.html", 16) == 0)
					{
						fs_open(&file, "/config.html");
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}

					/* Send an image file for requests to '/img/CISYNTH.png' */
					else if (strncmp((char const *)buf, "GET /img/CISYNTH.png", 20) == 0)
					{
						fs_open(&file, "/img/CISYNTH.png");
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}

					/* Send a favicon for requests to '/img/favicon_64x64.ico' */
					else if (strncmp((char const *)buf, "GET /img/favicon_64x64.ico", 26) == 0)
					{
						fs_open(&file, "/img/favicon_64x64.ico");
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}

					/* Get frequency data and send response */
					else if (strncmp((char const *)buf, "GET /getFreq", 12) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_var.cis_freq);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get DPI data and send response */
					else if (strncmp((char const *)buf, "GET /getDPI", 11) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_config.cis_dpi);

						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get and set oversampling settings */
					else if (strncmp((char const *)buf, "GET /getOversampling", 20) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_config.cis_oversampling);

						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get hand settings */
					else if (strncmp((char const *)buf, "GET /getHand", 12) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_config.cis_handedness);

						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get network settings */
					else if (strncmp((char const *)buf, "GET /getNetworkConfig", 21) == 0)
					{
						char response[400];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
								"{"
								"\"ip\":\"%d.%d.%d.%d\","
								"\"mask\":\"%d.%d.%d.%d\","
								"\"gw\":\"%d.%d.%d.%d\","
								"\"dest_ip\":\"%d.%d.%d.%d\","
								"\"udp_port\":%d"
								"}",
								shared_config.network_ip[0], shared_config.network_ip[1], shared_config.network_ip[2], shared_config.network_ip[3],
								shared_config.network_netmask[0], shared_config.network_netmask[1], shared_config.network_netmask[2], shared_config.network_netmask[3],
								shared_config.network_gw[0], shared_config.network_gw[1], shared_config.network_gw[2], shared_config.network_gw[3],
								shared_config.network_dest_ip[0], shared_config.network_dest_ip[1], shared_config.network_dest_ip[2], shared_config.network_dest_ip[3],
								shared_config.network_udp_port);

						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Send 404 if no route matches */
					else
					{
						fs_open(&file, "/404.html");
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}
				}
				else
				{
					/* Process POST request to set DPI */
					if (strncmp((char const *)buf, "POST /setDPI", 12) == 0)
					{
						char *dpiValue = strstr(buf, "dpi=") + 4;  // Point to the first character of the value

						if (dpiValue)
						{
							shared_config.cis_dpi = atoi(dpiValue);
							shared_config.cis_dpi  = shared_config.cis_dpi  < 200 ? 200 : shared_config.cis_dpi  > 200 ? 400 : shared_config.cis_dpi ;
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int) shared_config.cis_dpi);
							netconn_write(conn, response, len, NETCONN_COPY);

							cis_configure();
						}
						else
						{
							char *errorResponse = "Error: DPI value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
						}
					}

					/* Process POST request to set hand settings */
					else if (strncmp((char const *)buf, "POST /setHand", 13) == 0)
					{
						char *handValue = strstr(buf, "hand=") + 5;

						if (handValue)
						{
							shared_config.cis_handedness = atoi(handValue);
							shared_config.cis_handedness =  shared_config.cis_handedness < 0 ? 0 :  shared_config.cis_handedness > 1 ? 1 :  shared_config.cis_handedness;
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int) shared_config.cis_handedness);
							netconn_write(conn, response, len, NETCONN_COPY);
						}
						else
						{
							char *errorResponse = "Error: Hand value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
						}
					}

					/* Process POST request to set oversampling */
					else if (strncmp((char const *)buf, "POST /setOversampling", 21) == 0)
					{
						char *oversamplingValue = strstr(buf, "oversampling=") + 13;

						if (oversamplingValue)
						{
							shared_config.cis_oversampling = atoi(oversamplingValue);
							shared_config.cis_oversampling  = shared_config.cis_oversampling  < 0 ? 0 : shared_config.cis_oversampling  > 32 ? 32 : shared_config.cis_oversampling ;
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOversampling set to %d", (int)shared_config.cis_oversampling);
							netconn_write(conn, response, len, NETCONN_COPY);
						}
						else
						{
							char *errorResponse = "Error: Oversampling value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
						}
					}

					/* Process calibration start command */
					else if (strncmp((char const *)buf, "POST /startCalibration", 22) == 0)
					{
					    char *body = strstr(buf, "\r\n\r\n");
					    if (body && strstr(body, "CIS_CAL_START"))
					    {
					        shared_var.cis_cal_state = CIS_CAL_REQUESTED;

					        clock_t start_time = clock();
					        const double timeout = 10.0;

					        while (shared_var.cis_cal_state != CIS_CAL_END)
					        {
					            clock_t current_time = clock();
					            double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
					            if (elapsed_time >= timeout)
					            {
					                const char *error_response = "HTTP/1.1 408 Request Timeout\r\nContent-Type: text/plain\r\n\r\nCalibration timeout";
					                netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
					                break;
					            }
					            osDelay(1);
					        }

					        if (shared_var.cis_cal_state == CIS_CAL_END)
					        {
					            const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nCalibration started";
					            netconn_write(conn, response, strlen(response), NETCONN_COPY);
					        }
					    }
					}

					/* Handler for updating network settings */
					else if (strncmp((char const *)buf, "POST /updateNetworkConfig", 25) == 0)
					{
						char *data = strstr((char *)buf, "\r\n\r\n") + 4; // Assuming data starts after the header
						if (data)
						{
							int ip[4], mask[4], gw[4], dest_ip[4], udp_port;

							// Parsing POST data
							sscanf(data, "ip=%d.%d.%d.%d&mask=%d.%d.%d.%d&gateway=%d.%d.%d.%d&dest_ip=%d.%d.%d.%d&udp_port=%d",
									&ip[0], &ip[1], &ip[2], &ip[3],
									&mask[0], &mask[1], &mask[2], &mask[3],
									&gw[0], &gw[1], &gw[2], &gw[3],
									&dest_ip[0], &dest_ip[1], &dest_ip[2], &dest_ip[3],
									&udp_port);

							// Updating shared configuration
							for (int i = 0; i < 4; i++)
							{
								shared_config.network_ip[i] = ip[i];
								shared_config.network_netmask[i] = mask[i];
								shared_config.network_gw[i] = gw[i];
								shared_config.network_dest_ip[i] = dest_ip[i];
							}
							shared_config.network_udp_port = udp_port;

							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[200];
					        int len = sprintf(response,
					                "HTTP/1.1 200 OK\r\n"
					                "Content-Type: text/plain\r\n"
					                "Cache-Control: no-cache, no-store, must-revalidate\r\n"
					                "Pragma: no-cache\r\n"
					                "Expires: 0\r\n"
					                "\r\n"
					                "Network settings updated.");
							netconn_write(conn, response, len, NETCONN_COPY);

							char newIP[16];
							sprintf(newIP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

						    len = sprintf(response, "HTTP/1.1 302 Found\r\nLocation: http://%s/config.html\r\n\r\n", newIP);
							netconn_write(conn, response, len, NETCONN_COPY);

							reboot = true;
						}
						else
						{
							char response[100];
							int len = sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nInvalid request.");
							netconn_write(conn, response, len, NETCONN_COPY);
						}
					}

					/* start factory reset */
					else if (strncmp((char const *)buf, "POST /factoryReset", 18) == 0)
					{
						char *body = strstr(buf, "\r\n\r\n");
						const char *response;
						if (body && strstr(body, "START_FACTORY_RESET"))
						{

							if (file_factoryReset() != FILEMANAGER_OK)
							{
								response = "HTTP/1.1 500 Internal Server Error\r\n"
										"Content-Type: text/plain\r\n\r\n"
										"Error: Factory reset failed due to internal error";
							}
							else
							{
								response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nFactory reset done";
								reboot = true;
							}
							netconn_write(conn, response, strlen(response), NETCONN_COPY);
						}
					}
					/* firmware update */
					else if (strncmp((char const *)buf, "POST /upload", 12) == 0)
					{
					    int fw_ret = fwupdate_octetstream_state_machine(conn, buf, buflen);
#ifdef HTTP_SERVER_DEBUG
            printf("# fw_ret = %d, accum_length = %d\n", fw_ret, fwupdate.accum_length);
#endif
					    // On suppose ici que fwupdate_octetstream_state_machine utilise des variables statiques pour accumuler les données.
					    if (fw_ret == FWUPDATE_STATUS_NONE)
					    {
					        /* Rien à faire */
					    }
					    else if (fw_ret == FWUPDATE_STATUS_INPROGRESS)
					    {
					        // Ne pas fermer la connexion tant que le transfert n'est pas complet
					        close = false;
					    }
					    else if (fw_ret == FWUPDATE_STATUS_DONE)
					    {
					        close = true;
					        char response[100];
					        int len = sprintf(response,
					                          "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nUpdate Successful.\r\n");
					        netconn_write(conn, response, len, NETCONN_COPY);

					        // Finalisation, puis éventuellement redémarrage
					        STM32Flash_StatusTypeDef status = STM32Flash_writePersistentData(FW_UPDATE_RECEIVED);
					        if (status == STM32FLASH_OK)
					        {
					            printf("Firmware update received\n");
					        }
					        else
					        {
					            printf("Failed to write firmware update status in STM32 flash\n");
					        }
					        reboot = true;
					    }
					}
				}
				/* Process all data that may be present in the netbuf */
#ifdef HTTP_SERVER_DEBUG
				printf("# netbuf_next = %d\n", netbuf_next(inbuf));
#endif
			}
			while (netbuf_next(inbuf) >= 0);

			/* Delete the buffer (netconn_recv gives us ownership,
	         so we have to make sure to deallocate the buffer) */
			netbuf_delete(inbuf);
		}
		else
		{
#ifdef HTTP_SERVER_DEBUG
			printf("# netconn_recv error: %d %d\n", recv_err, netconn_err(conn));
#endif
		}
	    if (fwupdate.accum_length >= fwupdate.file_length)
	    {
	        break;
	    }
		if (close)
		{
#ifdef HTTP_SERVER_DEBUG
	printf("===== http_server_serve close\n");
#endif
			/* Action requires us to close the connection now instead of
	        blocking on the next netconn_recv. */
			netconn_close(conn);
			break;
		}
	} /* while netconn_recv */

	if (reboot)
	{
		netconn_close(conn);
		printf("Rebooting in 1\n");
		/* Wait 5 seconds. */
		osDelay(1000);
		NVIC_SystemReset();
	}

}

// Function to initialize and manage the HTTP server thread
static void http_thread(void *arg)
{
    struct netconn *conn, *newconn;
    err_t err, accept_err;

    // Create a new TCP connection handle
    conn = netconn_new(NETCONN_TCP);
    if (conn == NULL)
    {
        // If the connection handle cannot be created, exit the function
        printf("Error: Failed to create new TCP connection handle.\n");
        return;
    }

    // Bind the connection to port 80 and listen for incoming connections
    if (conn != NULL)
    {
        //netconn_set_sendtimeout(conn, 500);

        err_t err = netconn_bind(conn, IP_ADDR_ANY, 80);
        if (err == ERR_OK)
        {
            netconn_listen(conn);
#ifdef HTTP_SERVER_DEBUG
            printf("The server is now listening on port 80\n");
#endif
        }
        else
        {
#ifdef HTTP_SERVER_DEBUG
            printf("Failed to bind to port 80.\n");
#endif
        }
    }
    else
    {
        printf("Unable to create a netconn.\n");
    }

    // Start listening for incoming connections
    err = netconn_listen(conn);
    if (err != ERR_OK)
    {
        // If listening fails, print the error code and clean up
        printf("Error: netconn_listen failed with error code %d\n", err);
        netconn_delete(conn);
        return;
    }

#ifdef HTTP_SERVER_DEBUG
    printf("HTTP server is listening on port 80...\n");
#endif

    // Main loop: wait for and handle incoming connections
    while (1)
    {
        // Accept an incoming connection
        accept_err = netconn_accept(conn, &newconn);
        if (accept_err == ERR_OK)
        {
#ifdef HTTP_SERVER_DEBUG
            printf("New connection accepted.\n");
#endif

            // Handle the connection using a separate function
            http_server(newconn);

        	// Close the connection (server closes in HTTP)
            netconn_close(newconn);
            netconn_delete(newconn);

#ifdef HTTP_SERVER_DEBUG
            printf("Connection closed.\n");
#endif
        }
        else
        {
            // Print an error message if the connection accept fails
            printf("Error: netconn_accept failed with error code %d\n", accept_err);
        }

        // Small delay to avoid hogging CPU
        osDelay(1); // 1 ms delay, adjust as needed
    }

    // Clean up the main connection handle (should never reach here)
    netconn_delete(conn);
}

HTTPSERVER_StatusTypeDef http_serverInit()
{
	if (xTaskCreate(http_thread, "http_thread", 8192, NULL, osPriorityLow, &http_ThreadHandle) == pdPASS)
	{
		//printf("HTTP initialisation SUCCESS\n");
		return HTTPSERVER_OK;
	}
	else
	{
		//printf("Failed to create http task.\n");
		return HTTPSERVER_ERROR;
	}
}
