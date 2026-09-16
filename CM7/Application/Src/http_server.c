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
#include "cis_linearCal.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"

#include "ff.h" // FATFS include
#include "diskio.h" // DiskIO include

#include "file_manager.h"
#include "cis.h"
#include "icm42688.h"

#include "stm32_flash.h"

#include "http_server.h"
#include "cis_scan.h"
#include "ota_app.h"
#include "link_server.h"
#include "sys_identity.h"
#include "log_ring.h"
#include "boot_mailbox.h"
#include "udp_client.h"
#include "sp3ctra_link.h"
#include "FreeRTOS.h"
#include "task.h"

/* Receive timeout for an accepted connection (ms). Browsers (Safari in particular) open idle
 * "preconnect" keep-alive sockets: without a timeout the single-threaded server blocks forever in
 * netconn_recv(), the accept backlog fills up and every new request is answered with a TCP RST. */
#define HTTP_RECV_TIMEOUT_MS 2000

/* One complete HTTP request is assembled here before being parsed: browsers (Safari XHR in
 * particular) send the POST headers and the body in two separate TCP segments, i.e. two netbufs.
 * The buffer is also NUL-terminated so that strstr()/strncmp() on the request are safe. */
#define HTTP_REQ_BUF_SIZE 2048
static char http_reqbuf[HTTP_REQ_BUF_SIZE];

/* Requests served on one keep-alive connection before we close it anyway: at the
 * live viewer's 25 polls per second, 64 is ~2.5 s of viewing (see GET /scan.bin). */
#define HTTP_KEEPALIVE_MAX_REQUESTS 64

/* Same, for the live scan. The server handles ONE connection at a time, so this
 * burst length is also the ceiling on every OTHER client's wait in the accept
 * queue: at 512 (~20 s at 25 polls/s) the settings XHRs of the scan page itself
 * starved forever behind their own waterfall. ~2 s keeps the viewer smooth (the
 * since= batching resumes without losing lines) and bounds settings latency. */
#define HTTP_KEEPALIVE_MAX_SCAN_REQUESTS 50

/* Copy the whole netbuf chain into http_reqbuf, then for a POST keep receiving (bounded by the
 * receive timeout) until the Content-Length body is complete. Returns the assembled length. */
/* netconn_recv() allocates a netbuf from MEMP_NETBUF *before* waiting for data and returns
 * ERR_MEM at once when the pool is momentarily empty (the UDP scanline sender allocates one per
 * packet, 13 k/s): retry briefly instead of dropping the request. */
static err_t http_recv(struct netconn *conn, struct netbuf **inbuf)
{
    err_t e = ERR_MEM;
    for (int retry = 0; retry < 100 && e == ERR_MEM; retry++)
    {
        e = netconn_recv(conn, inbuf);
        if (e == ERR_MEM)
        {
            osDelay(1);
        }
    }
    return e;
}

static u16_t http_assembleRequest(struct netconn *conn, struct netbuf *inbuf)
{
    u16_t total = 0;
    struct netbuf *nb = inbuf;
    int extra = 0;

    for (;;)
    {
        char *p;
        u16_t l;
        do
        {
            netbuf_data(nb, (void **)&p, &l);
            if ((u32_t)total + l > HTTP_REQ_BUF_SIZE - 1)
            {
                l = (u16_t)(HTTP_REQ_BUF_SIZE - 1 - total);
            }
            memcpy(http_reqbuf + total, p, l);
            total += l;
        } while (netbuf_next(nb) >= 0);
        if (nb != inbuf)
        {
            netbuf_delete(nb);
        }
        http_reqbuf[total] = '\0';

        if (strncmp(http_reqbuf, "POST ", 5) != 0 || total >= HTTP_REQ_BUF_SIZE - 1 || extra >= 4)
        {
            break;
        }
        char *hdr_end = strstr(http_reqbuf, "\r\n\r\n");
        if (hdr_end != NULL)
        {
            long need = 0;
            char *cl = strstr(http_reqbuf, "Content-Length:");
            if (cl != NULL && cl < hdr_end)
            {
                need = atol(cl + 15);
            }
            long have = (long)total - (long)((hdr_end + 4) - http_reqbuf);
            if (have >= need)
            {
                break;
            }
        }
        {
            err_t e = http_recv(conn, &nb);
            if (e != ERR_OK)
            {
                printf("HTTP: request body not received (err=%d, %u bytes so far)\n", (int)e, (unsigned)total);
                break; /* timeout or connection closed: parse what we have */
            }
        }
        extra++;
    }
    return total;
}

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

typedef struct
{
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

    /* New: store the boundary from the HTTP header here (if multipart/form-data). */
    char boundary[128];

    /* Store the filename and full path for later use */
    char file_name[FILE_NAME_MAX_LENGTH];
    char full_file_path[FILE_NAME_MAX_LENGTH];

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

/**
 * @brief  Ramene la machine a etats du televersement a son point de depart.
 *
 * Un televersement se deroule entierement dans une seule connexion : la boucle
 * netconn_recv de http_server() est maintenue ouverte tant qu'il dure. Une
 * connexion qui tombe en cours de route laissait la machine en
 * FWUPDATE_STATE_DOWNLOAD_STREAM avec son FIL ouvert, et la requete suivante --
 * fut-ce un simple GET -- etait ecrite dans le fichier firmware jusqu'a
 * atteindre file_length, ce qui declenchait un redemarrage sur un paquet
 * corrompu. On repart donc d'un etat propre a chaque connexion.
 */
static void fwupdate_abort(void)
{
    if (fwupdate.has_been_initialized)
    {
        printf("@ fwupdate - discarding an unfinished upload (%d/%d bytes)\n",
               fwupdate.accum_length, fwupdate.file_length);
        f_close(&file);
    }

    memset(&fwupdate, 0, sizeof(fwupdate));
    fwupdate.state = FWUPDATE_STATE_HEADER;
}

/**
 * @brief  Cloture un televersement complet : verification, reponse HTTP, verdict.
 *
 * Le paquet est verifie ICI, avant tout redemarrage : un paquet invalide est
 * refuse avec sa raison dans la reponse et ne coute aucun reboot. L'ancienne
 * sequence levait le drapeau sans rien verifier, et c'est le bootloader qui
 * decouvrait le probleme deux redemarrages plus tard.
 *
 * @retval true si l'appareil doit redemarrer pour appliquer le paquet.
 */
static bool fwupdate_finish(struct netconn *conn)
{
    const char *reason = otaApp_acceptPackage(fwupdate.full_file_path);

    if (reason == NULL)
    {
        const char *accepted = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/plain\r\n\r\n"
                               "Update accepted, rebooting.\r\n";

        printf("Firmware package accepted, rebooting to apply it\n");
        netconn_write(conn, accepted, strlen(accepted), NETCONN_COPY);
        return true;
    }

    char rejected[192];
    int rejectedLen = snprintf(rejected, sizeof(rejected),
                               "HTTP/1.1 400 Bad Request\r\n"
                               "Content-Type: text/plain\r\n\r\n"
                               "Firmware rejected: %s\r\n", reason);

    printf("Firmware package rejected: %s\n", reason);
    netconn_write(conn, rejected, (size_t)rejectedLen, NETCONN_COPY);

    f_unlink(fwupdate.full_file_path);
    return false;
}

static int fwupdate_multipart_state_machine(struct netconn *conn, char *buf, u16_t buflen)
{
    int ret = FWUPDATE_STATUS_NONE;
    char *buf_start = buf;
    char *buf_end = buf + buflen; // Points to byte AFTER end of buffer.

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
        printf("@ fwupdate buf_start=%p, buf=%p buf_end=%p state=%d\n",
               (void*)buf_start, (void*)buf, (void*)buf_end, fwupdate.state);
#endif

        switch (fwupdate.state)
        {
            case FWUPDATE_STATE_HEADER:
            {
                /* Look for "POST /upload" to confirm an upload request */
                if ((buflen >= 12) && (strncmp(buf, "POST /upload", 12) == 0))
                {
                    ret = FWUPDATE_STATUS_ERROR; // Default to error until fully validated
                    printf("@ fwupdate - Scanning HEADER\n");

                    /* Parse Content-Length */
                    char *cl_ptr = strstr(buf, CONTENT_LENGTH_TAG);
                    if (cl_ptr)
                    {
                        cl_ptr += strlen(CONTENT_LENGTH_TAG);
                        fwupdate.content_length = atoi(cl_ptr);

                        /* We also parse the boundary from the Content-Type line if present */
                        {
                            /* Example line: "Content-Type: multipart/form-data; boundary=------MyBoundary" */
                            char *ct_ptr = strstr(buf, "Content-Type:");
                            if (ct_ptr)
                            {
                                char *boundary_pos = strstr(ct_ptr, "boundary=");
                                if (boundary_pos)
                                {
                                    boundary_pos += strlen("boundary=");
                                    /* Copy up to next space, CR, or semicolon, but not past our boundary buffer. */
                                    int i = 0;
                                    while (*boundary_pos && *boundary_pos != '\r' && *boundary_pos != '\n'
                                           && *boundary_pos != ' ' && *boundary_pos != ';'
                                           && i < (int)(sizeof(fwupdate.boundary) - 1))
                                    {
                                        fwupdate.boundary[i++] = *boundary_pos++;
                                    }
                                    fwupdate.boundary[i] = '\0'; // Null-terminate
                                }
                                else
                                {
                                    /* If there's no boundary, fallback to an empty string */
                                    fwupdate.boundary[0] = '\0';
                                }
                            }
                        }

                        /* Find the empty line that separates headers from body */
                        char *sep_ptr = strstr(cl_ptr, EMPTY_LINE_TAG);
                        if (sep_ptr)
                        {
                            sep_ptr += strlen(EMPTY_LINE_TAG);
                            buf_start = sep_ptr;
                            printf("@ fwupdate - Found content length = %d\n", fwupdate.content_length);

                            /* Next, parse the Content-Disposition to extract the filename */
                            char *disp_ptr = strstr(sep_ptr, "Content-Disposition:");
                            if (disp_ptr)
                            {
                                int extracted = sscanf(disp_ptr,
                                                       "Content-Disposition: form-data; name=\"firmware\"; filename=\"%255[^\"]\"",
                                                       fwupdate.file_name);
                                if (extracted == 1)
                                {
                                    /* Construct full path: FW_PATH + "/" + filename */
                                    size_t needed_length = strlen(FW_PATH) + 1 + strlen(fwupdate.file_name) + 1;
                                    if (needed_length < sizeof(fwupdate.full_file_path))
                                    {
                                        snprintf(fwupdate.full_file_path,
                                                 sizeof(fwupdate.full_file_path),
                                                 "%s/%s",
                                                 FW_PATH,
                                                 fwupdate.file_name);

                                        fwupdate.state = FWUPDATE_STATE_DOWNLOAD_START;
                                        ret = FWUPDATE_STATUS_INPROGRESS;
                                    }
                                    else
                                    {
                                        printf("@ fwupdate - File path too long\n");
                                        ret = FWUPDATE_STATUS_ERROR;
                                    }
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
                        else
                        {
                            printf("@ fwupdate - Error extracting empty line tag\n");
                            ret = FWUPDATE_STATUS_ERROR;
                        }
                    }
                }
                else
                {
                    buf = NULL; // Stop processing
                }
                break;
            }

            case FWUPDATE_STATE_DOWNLOAD_START:
            {
                const char *tags[] = { DOWNLOAD_STREAM_TAG, DOWNLOAD_STREAM_TAG_2 };
                ret = FWUPDATE_STATUS_ERROR;

                /* Create or overwrite the target file */
                FRESULT fr = f_open(&file, fwupdate.full_file_path, FA_WRITE | FA_CREATE_ALWAYS);
                if (fr != FR_OK)
                {
                    fwupdate.code = fr;
                    ret = FWUPDATE_STATUS_ERROR;
                    break;
                }
                f_close(&file);

                /* Look for either of the known stream tags to jump to the actual binary data. */
                for (int i = 0; i < (int)(sizeof(tags) / sizeof(tags[0])); ++i)
                {
                    char *found_tag = strstr(buf, tags[i]);
                    if (found_tag)
                    {
                        found_tag += strlen(tags[i]);
                        buf = found_tag;

                        /* The "file_length" is the total content minus the headers we've parsed out. */
                        size_t header_length = buf - buf_start;
                        fwupdate.file_length = fwupdate.content_length - (int)header_length;

#ifdef HTTP_SERVER_DEBUG
                        printf("@ fwupdate content len=%d, file len=%d, header len=%u\n",
                               fwupdate.content_length,
                               fwupdate.file_length,
                               (unsigned)header_length);
#endif

                        fwupdate.state = FWUPDATE_STATE_DOWNLOAD_STREAM;
                        fwupdate.accum_length = 0;
                        ret = FWUPDATE_STATUS_INPROGRESS;
                        break;
                    }
                }

                if (ret == FWUPDATE_STATUS_ERROR)
                {
                    printf("@ fwupdate - Error extracting file length / tags\n");
                }
                break;
            }

            case FWUPDATE_STATE_DOWNLOAD_STREAM:
            {
                if ((buf_end - buf) > 0)
                {
                    FRESULT fr;
                    UINT bytes_written;
                    ret = FWUPDATE_STATUS_INPROGRESS;

                    uint32_t data_len = (uint32_t)(buf_end - buf);

                    if (!fwupdate.has_been_initialized)
                    {
                        fr = f_open(&file, fwupdate.full_file_path, FA_WRITE | FA_CREATE_ALWAYS);
                        if (fr != FR_OK)
                        {
                            fwupdate.code = fr;
                            ret = FWUPDATE_STATUS_ERROR;
                            break;
                        }
                        fwupdate.has_been_initialized = 1;
                        fwupdate.stage = FW_UPDATE_Stage_IN_PROGRESS;
                    }

                    /* Write the data to the file */
                    fr = f_write(&file, buf, data_len, &bytes_written);
                    if ((fr != FR_OK) || (bytes_written != data_len))
                    {
                        fwupdate.code = fr;
                        f_close(&file);
                        ret = FWUPDATE_STATUS_ERROR;
                        break;
                    }

                    fwupdate.accum_length += data_len;
#ifdef HTTP_SERVER_DEBUG
                    printf("@ fwupdate accumBytes=%d\n", (int)fwupdate.accum_length);
#endif

                    /* If we've received all the expected data, let's see if there's a trailing boundary to remove. */
                    if (fwupdate.accum_length >= fwupdate.file_length)
                    {
                        /* Close the file first to ensure all data is written */
                        f_close(&file);

                        /* If we previously parsed a boundary, let's try to remove it from the end of the file. */
                        if (fwupdate.boundary[0] != '\0')
                        {
                            /* Read the last part of the file to check for boundary */
                            const size_t check_size = 512; /* Increased buffer for safety */
                            uint8_t *check_buffer = pvPortMalloc(check_size);

                            if (check_buffer != NULL)
                            {
                                /* Reopen file for reading */
                                FRESULT fr = f_open(&file, fwupdate.full_file_path, FA_READ | FA_WRITE);
                                if (fr == FR_OK)
                                {
                                    /* Get actual file size */
                                    DWORD actual_size = f_size(&file);
                                    DWORD read_offset = (actual_size > check_size) ? (actual_size - check_size) : 0;
                                    UINT bytes_to_read = (actual_size > check_size) ? check_size : actual_size;
                                    UINT bytes_read;

                                    /* Seek to position and read */
                                    f_lseek(&file, read_offset);
                                    fr = f_read(&file, check_buffer, bytes_to_read, &bytes_read);

                                    /* La recherche du boundary compare des blocs
                                     * de boundary_len octets : sans le garde-fou
                                     * ajoute aux deux boucles ci-dessous, un
                                     * fichier plus court que le boundary rendait
                                     * bytes_read - boundary_len negatif en size_t,
                                     * soit quatre milliards d'iterations hors du
                                     * tampon de 512 octets. */
                                    if (fr == FR_OK && bytes_read > 0)
                                    {
                                        /* Build the complete boundary string with ending -- */
                                        char boundary_final[256];
                                        snprintf(boundary_final, sizeof(boundary_final), "\r\n--%s--", fwupdate.boundary);
                                        size_t boundary_len = strlen(boundary_final);

                                        /* Search for boundary in the buffer */
                                        bool found = false;
                                        size_t new_file_size = actual_size;

                                        for (size_t i = 0; boundary_len <= bytes_read && i <= bytes_read - boundary_len; i++)
                                        {
                                            if (memcmp(&check_buffer[i], boundary_final, boundary_len) == 0)
                                            {
                                                /* Found the boundary - calculate new file size */
                                                new_file_size = read_offset + i;
                                                found = true;

                                                printf("@ fwupdate - Found boundary at position %u, truncating file from %u to %u bytes\n",
                                                       (unsigned int)(read_offset + i),
                                                       (unsigned int)actual_size,
                                                       (unsigned int)new_file_size);

                                                /* Truncate the file */
                                                f_lseek(&file, new_file_size);
                                                f_truncate(&file);

                                                /* Update the tracked file length */
                                                fwupdate.file_length = new_file_size;
                                                break;
                                            }
                                        }

                                        if (!found)
                                        {
                                            /* Also check for boundary without the ending \r\n (edge case) */
                                            snprintf(boundary_final, sizeof(boundary_final), "--%s--", fwupdate.boundary);
                                            boundary_len = strlen(boundary_final);

                                            for (size_t i = 0; boundary_len <= bytes_read && i <= bytes_read - boundary_len; i++)
                                            {
                                                if (memcmp(&check_buffer[i], boundary_final, boundary_len) == 0)
                                                {
                                                    new_file_size = read_offset + i;

                                                    printf("@ fwupdate - Found boundary (without CRLF) at position %u, truncating file\n",
                                                           (unsigned int)(read_offset + i));

                                                    f_lseek(&file, new_file_size);
                                                    f_truncate(&file);
                                                    fwupdate.file_length = new_file_size;
                                                    break;
                                                }
                                            }
                                        }
                                    }

                                    f_close(&file);
                                }

                                vPortFree(check_buffer);
                            }
                        }

                        /* Clean up and remove older firmware if needed */
                        delete_old_firmware(fwupdate.file_name);
                        fwupdate.has_been_initialized = 0;
                        fwupdate.stage = FW_UPDATE_Stage_VERIFIED;
                    }

                    buf = NULL;

                    if (ret == FWUPDATE_STATUS_INPROGRESS)
                    {
                        /* If the file was fully received */
                        if ((fwupdate.accum_length >= fwupdate.file_length)
                            || (fwupdate.stage == FW_UPDATE_Stage_VERIFIED))
                        {
                            /* Le fichier est complet, mais rien ne dit encore
                             * qu'il soit valide : la reponse HTTP est laissee a
                             * l'appelant, qui la choisit apres verification.
                             * Repondre 200 ici puis 400 apres coup produisait
                             * deux reponses pour une seule requete. */
                            ret = FWUPDATE_STATUS_DONE;

                            /* Reset state for a new download next time */
                            fwupdate.state = FWUPDATE_STATE_HEADER;
                        }
                    }
                }
                break;
            }
        } /* switch (fwupdate.state) */
    } /* while (buf && buf < buf_end) */

    if (ret == FWUPDATE_STATUS_ERROR)
    {
        fwupdate_abort();
    }

    return ret;
}
/**
 * @brief  Recoit un televersement de firmware de bout en bout.
 *
 * Ce gestionnaire prend la main sur la connexion, car http_assembleRequest() ne
 * peut pas servir ici : son tampon de 2 Ko tronque silencieusement tout ce qui
 * depasse, si bien qu'un paquet de plusieurs centaines de kilo-octets
 * n'atteignait jamais sa taille annoncee et que le televersement ne se
 * terminait jamais.
 *
 * Les en-tetes -- qui peuvent s'etaler sur plusieurs segments TCP, ce pour quoi
 * l'assembleur existe -- sont accumules dans http_reqbuf jusqu'a la balise de
 * flux, avec terminaison par un NUL pour que les strstr() de la machine a etats
 * restent bornes. Passe ce point, chaque fragment part directement dans le
 * fichier : la machine a etats est alors en DOWNLOAD_STREAM, ou elle ne fait
 * aucune operation de chaine sur le tampon.
 *
 * @param  conn   connexion, drainee jusqu'a Content-Length ou jusqu'au timeout
 *                de reception.
 * @param  inbuf  premier netbuf recu. Il reste la propriete de l'appelant, qui
 *                le libere ; ceux obtenus ensuite sont liberes ici.
 * @retval true si l'appareil doit redemarrer pour appliquer le paquet.
 */
static bool fwupdate_handleUpload(struct netconn *conn, struct netbuf *inbuf)
{
    struct netbuf *nb = inbuf;
    bool owned = false; /* inbuf appartient a l'appelant */
    bool headerDone = false;
    u16_t assembled = 0;
    int ret = FWUPDATE_STATUS_INPROGRESS;

    for (;;)
    {
        do
        {
            char *p;
            u16_t l;

            if (netbuf_data(nb, (void **)&p, &l) != ERR_OK)
            {
                break;
            }

            if (headerDone)
            {
                ret = fwupdate_multipart_state_machine(conn, p, l);
            }
            else if ((u32_t)assembled + l > HTTP_REQ_BUF_SIZE - 1)
            {
                /* Les en-tetes multipart tiennent largement dans deux segments.
                 * Au-dela, mieux vaut refuser franchement que recopier un
                 * paquet ampute -- c'etait exactement le defaut de l'assembleur. */
                printf("@ fwupdate - headers exceed %d bytes, aborting\n",
                       HTTP_REQ_BUF_SIZE - 1);
                ret = FWUPDATE_STATUS_ERROR;
            }
            else
            {
                memcpy(http_reqbuf + assembled, p, l);
                assembled = (u16_t)(assembled + l);
                http_reqbuf[assembled] = '\0';

                /* La balise de flux marque la fin des en-tetes multipart :
                 * tant qu'elle manque, il reste des en-tetes a recevoir. */
                if (strstr(http_reqbuf, DOWNLOAD_STREAM_TAG) != NULL ||
                    strstr(http_reqbuf, DOWNLOAD_STREAM_TAG_2) != NULL)
                {
                    ret = fwupdate_multipart_state_machine(conn, http_reqbuf, assembled);
                    headerDone = true;
                }
            }

            if (ret != FWUPDATE_STATUS_INPROGRESS)
            {
                goto finished;
            }
        }
        while (netbuf_next(nb) >= 0);

        if (owned)
        {
            netbuf_delete(nb);
            owned = false;
        }

        if (http_recv(conn, &nb) != ERR_OK)
        {
            printf("@ fwupdate - upload interrupted after %d/%d bytes\n",
                   fwupdate.accum_length, fwupdate.file_length);
            ret = FWUPDATE_STATUS_ERROR;
            goto finished;
        }
        owned = true;
    }

finished:
    if (owned)
    {
        netbuf_delete(nb);
    }

    if (ret == FWUPDATE_STATUS_DONE)
    {
        return fwupdate_finish(conn);
    }

    fwupdate_abort();
    return false;
}

/* ---- Navigateur de fichiers en lecture seule (remplace le serveur FTP) ----
 *
 * Deux points d'entree : GET /fs/list?path= (liste JSON d'un repertoire) et
 * GET /fs/get?path= (telechargement d'un fichier). Aucune ecriture n'est
 * possible par ce canal : deposer un firmware reste l'affaire de POST /upload. */

/* Le telechargement passe par ces statiques plutot que par la pile de la
 * tache : un FIL pese plus d'un demi-kilo-octet avec les noms longs, et le
 * serveur est monotache, un seul jeu suffit (meme motif que http_reqbuf). */
static FIL fsbrowse_file;
static uint8_t fsbrowse_chunk[2048];

static int fsbrowse_hexVal(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Extrait et decode l'argument "path=" de la requete vers out, prefixe "0:"
 * (syntaxe FatFs). Refuse ".." -- FatFs accepte les chemins relatifs
 * (_FS_RPATH) et rien ne doit permettre de sortir du volume -- ainsi que tout
 * caractere de controle. Retourne false si le chemin manque ou est malforme. */
static bool fsbrowse_extractPath(const char *req, char *out, size_t outsz)
{
    const char *p = strstr(req, "path=");
    if (p == NULL)
    {
        return false;
    }
    p += 5;

    size_t n = 2;
    out[0] = '0'; out[1] = ':';
    while (*p != '\0' && *p != ' ' && *p != '&' && *p != '\r' && *p != '\n')
    {
        char c = *p++;
        if (c == '%')
        {
            int hi = fsbrowse_hexVal(p[0]);
            int lo = (hi >= 0) ? fsbrowse_hexVal(p[1]) : -1;
            if (lo < 0)
            {
                return false;
            }
            c = (char)((hi << 4) | lo);
            p += 2;
        }
        else if (c == '+')
        {
            c = ' ';
        }
        if ((unsigned char)c < 0x20 || n >= outsz - 1)
        {
            return false;
        }
        out[n++] = c;
    }
    out[n] = '\0';

    if (out[2] != '/' || strstr(out, "..") != NULL)
    {
        return false;
    }
    /* FatFs ouvre la racine avec "0:/" mais refuse un slash final ailleurs. */
    if (n > 3 && out[n - 1] == '/')
    {
        out[n - 1] = '\0';
    }
    return true;
}

static void fsbrowse_sendError(struct netconn *conn, const char *status, const char *text)
{
    char response[160];
    int len = snprintf(response, sizeof(response),
                       "HTTP/1.1 %s\r\nContent-Type: text/plain\r\nContent-Length: %u\r\n\r\n%s",
                       status, (unsigned)strlen(text), text);
    netconn_write(conn, response, (size_t)len, NETCONN_COPY);
}

/* Liste un repertoire en JSON : {"entries":[{"n":nom,"d":0|1,"s":octets},...]}.
 * Le corps est emis entree par entree, sans Content-Length : la connexion est
 * fermee apres la reponse (close reste vrai), ce qui en marque la fin. */
static void fsbrowse_list(struct netconn *conn, const char *req)
{
    char path[FILE_NAME_MAX_LENGTH];
    DIR dir;

    if (!fsbrowse_extractPath(req, path, sizeof(path)))
    {
        fsbrowse_sendError(conn, "400 Bad Request", "Bad path");
        return;
    }
    if (f_opendir(&dir, path) != FR_OK)
    {
        fsbrowse_sendError(conn, "404 Not Found", "No such directory");
        return;
    }

    const char *hdr = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json\r\n"
                      "Cache-Control: no-store\r\n"
                      "Connection: close\r\n\r\n"
                      "{\"entries\":[";
    netconn_write(conn, hdr, strlen(hdr), NETCONN_COPY);

    FILINFO fno;
    bool first = true;
    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0')
    {
        /* Nom echappe pour JSON : seuls " et \ peuvent surgir d'un nom FatFs,
         * les caracteres de controle y sont interdits. */
        char name[FILE_NAME_MAX_LENGTH * 2];
        size_t w = 0;
        for (const char *r = fno.fname; *r != '\0' && w < sizeof(name) - 2; r++)
        {
            if (*r == '"' || *r == '\\')
            {
                name[w++] = '\\';
            }
            name[w++] = *r;
        }
        name[w] = '\0';

        char entry[sizeof(name) + 48];
        int len = snprintf(entry, sizeof(entry), "%s{\"n\":\"%s\",\"d\":%d,\"s\":%lu}",
                           first ? "" : ",", name,
                           (fno.fattrib & AM_DIR) ? 1 : 0,
                           (unsigned long)fno.fsize);
        if (netconn_write(conn, entry, (size_t)len, NETCONN_COPY) != ERR_OK)
        {
            break;
        }
        first = false;
    }
    f_closedir(&dir);

    netconn_write(conn, "]}", 2, NETCONN_COPY);
}

/* Sert un fichier du volume en telechargement (Content-Disposition:
 * attachment), par morceaux de sizeof(fsbrowse_chunk). */
static void fsbrowse_download(struct netconn *conn, const char *req)
{
    char path[FILE_NAME_MAX_LENGTH];

    if (!fsbrowse_extractPath(req, path, sizeof(path)))
    {
        fsbrowse_sendError(conn, "400 Bad Request", "Bad path");
        return;
    }
    if (f_open(&fsbrowse_file, path, FA_READ) != FR_OK)
    {
        fsbrowse_sendError(conn, "404 Not Found", "No such file");
        return;
    }

    /* Nom propose au navigateur : la derniere composante du chemin, ses
     * eventuels guillemets neutralises pour ne pas casser l'en-tete. */
    const char *fileName = strrchr(path, '/');
    fileName = (fileName != NULL) ? fileName + 1 : path + 2;
    char safeName[FILE_NAME_MAX_LENGTH];
    size_t w = 0;
    for (const char *r = fileName; *r != '\0' && w < sizeof(safeName) - 1; r++)
    {
        safeName[w++] = (*r == '"') ? '_' : *r;
    }
    safeName[w] = '\0';

    char hdr[FILE_NAME_MAX_LENGTH + 192];
    int hdrLen = snprintf(hdr, sizeof(hdr),
                          "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/octet-stream\r\n"
                          "Content-Length: %lu\r\n"
                          "Content-Disposition: attachment; filename=\"%s\"\r\n"
                          "Cache-Control: no-store\r\n"
                          "Connection: close\r\n\r\n",
                          (unsigned long)f_size(&fsbrowse_file), safeName);
    if (netconn_write(conn, hdr, (size_t)hdrLen, NETCONN_COPY) == ERR_OK)
    {
        UINT rd;
        while (f_read(&fsbrowse_file, fsbrowse_chunk, sizeof(fsbrowse_chunk), &rd) == FR_OK && rd > 0)
        {
            if (netconn_write(conn, fsbrowse_chunk, rd, NETCONN_COPY) != ERR_OK)
            {
                break; /* client parti : le corps s'arrete, la connexion se ferme */
            }
        }
    }
    f_close(&fsbrowse_file);
}

/* ------------------------------------------------------------------------
 * Journal en RAM retenue et passage en mode flasheur reseau (docs/NETBOOT.md)
 * ------------------------------------------------------------------------ */

/* GET /log?src=cm7|cm4&since=N&max=M : texte brut depuis la position N ;
 * sans since, les M derniers octets. X-Log-Next donne la position a redemander. */
static uint8_t logserve_body[4096];

static void logserve_get(struct netconn *conn, const char *req)
{
	char line[256];
	char header[192];
	const char *eol = strstr(req, "\r\n");
	size_t n = eol ? (size_t)(eol - req) : strlen(req);
	log_src_t src = LOG_SRC_CM7;
	uint32_t since = 0, next = 0, got, head;
	uint32_t max = sizeof(logserve_body);
	bool have_since = false;
	const char *a;
	int hl;

	/* Les parametres ne sont cherches que dans la ligne de requete. */
	if (n > sizeof(line) - 1) n = sizeof(line) - 1;
	memcpy(line, req, n);
	line[n] = '\0';

	if (strstr(line, "src=cm4") != NULL) src = LOG_SRC_CM4;
	if ((a = strstr(line, "since=")) != NULL) { since = strtoul(a + 6, NULL, 10); have_since = true; }
	if ((a = strstr(line, "max=")) != NULL)
	{
		max = strtoul(a + 4, NULL, 10);
		if (max > sizeof(logserve_body)) max = sizeof(logserve_body);
	}

	head = log_ring_head(src);
	if (!have_since) since = (head > max) ? head - max : 0;
	got = log_ring_read(src, since, logserve_body, max, &next);

	hl = snprintf(header, sizeof(header),
	              "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\n"
	              "Cache-Control: no-store\r\nX-Log-Next: %lu\r\nX-Log-Head: %lu\r\n"
	              "Content-Length: %lu\r\nConnection: close\r\n\r\n",
	              (unsigned long)next, (unsigned long)head, (unsigned long)got);
	netconn_write(conn, header, (size_t)hl, NETCONN_COPY);
	if (got > 0) netconn_write(conn, logserve_body, got, NETCONN_COPY);
}

/* POST /netboot : demande consommee apres la reponse, hors de la boucle de
 * lecture, pour que la fermeture TCP parte avant le reset. */
static bool netboot_requested = false;

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
	int served = 0;

#ifdef HTTP_SERVER_DEBUG
	printf("===== http_server_serve recv\n");
#endif

    if (conn == NULL)
    {
        printf("Error: Null connection passed to http_server.\n");
        return;
    }

	/* Etat propre a chaque connexion : voir fwupdate_abort(). */
	fwupdate_abort();

	/* Read the data from the port, blocking if nothing yet there.
	   We assume the request (the part we care about) is in one netbuf */
	while ((recv_err = http_recv(conn, &inbuf)) == ERR_OK)
	{
		if (netconn_err(conn) == ERR_OK)
		{
			do {
				char *first;
				u16_t firstlen;

				/* Le televersement de firmware est detourne AVANT l'assembleur :
				 * son tampon de 2 Ko ne peut pas contenir un paquet, et le
				 * tronquer empechait la fin du transfert d'etre detectee. */
				if (netbuf_data(inbuf, (void **)&first, &firstlen) == ERR_OK &&
				    firstlen >= 12 && strncmp(first, "POST /upload", 12) == 0)
				{
					reboot = fwupdate_handleUpload(conn, inbuf);
					close = true;
					break;
				}

				/* Assemble headers + body (may span several netbufs) into http_reqbuf */
				buflen = http_assembleRequest(conn, inbuf);
				buf = http_reqbuf;
#ifdef HTTP_SERVER_DEBUG
				printf("# Process buffer: %p %d bytes\n", buf, buflen);
#endif

				/* Is this an HTTP GET command? (only check the first 5 chars, since
	            there are other formats for GET, and we're keeping it very simple )*/
				if ((buflen >= 5) && (strncmp(buf, "GET /", 5) == 0))
				{

					/* Check for various paths and handle GET requests.
					 * Static files (pages, stylesheet, script, images) are served by the
					 * catch-all at the end of this chain; only the dynamic endpoints are
					 * matched by name here. */
					if (strncmp((char const *)buf, "GET /getFreq", 12) == 0)
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

					/* Etat de la calibration du point noir. AVANT /getBlackPoint : meme prefixe. */
					else if (strncmp((char const *)buf, "GET /getBlackPointCalStatus", 27) == 0)
					{
						const char *st = (cisBlackPointCalState == 1U) ? "running"
						               : (cisBlackPointCalState == 2U) ? "done"
						               : (cisBlackPointCalState == 3U) ? "failed" : "idle";
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%s %d",
						                  st, (int)shared_config.cis_black_point);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Point noir de sortie (x1000 lineaire) : 0 physique, 55 dessin, ~25 photo */
					else if (strncmp((char const *)buf, "GET /getBlackPoint", 18) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_config.cis_black_point);

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
						int len = sprintf(response,
								"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
								"{"
								"\"ip\":\"%d.%d.%d.%d\","
								"\"mask\":\"%d.%d.%d.%d\","
								"\"gw\":\"%d.%d.%d.%d\","
								"\"dest_ip\":\"%d.%d.%d.%d\","
								"\"udp_port\":%d,"
								"\"link_port\":%d,"
								"\"stream_when_unbound\":%d"
								"}",
								shared_config.network_ip[0], shared_config.network_ip[1], shared_config.network_ip[2], shared_config.network_ip[3],
								shared_config.network_netmask[0], shared_config.network_netmask[1], shared_config.network_netmask[2], shared_config.network_netmask[3],
								shared_config.network_gw[0], shared_config.network_gw[1], shared_config.network_gw[2], shared_config.network_gw[3],
								shared_config.network_dest_ip[0], shared_config.network_dest_ip[1], shared_config.network_dest_ip[2], shared_config.network_dest_ip[3],
								shared_config.network_udp_port,
								shared_config.network_link_port,
								shared_config.stream_when_unbound);

						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get gyro sensitivity */
					else if (strncmp((char const *)buf, "GET /getGyroSensitivity", 23) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.imu_gyro_sensitivity);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get accel sensitivity */
					else if (strncmp((char const *)buf, "GET /getAccelSensitivity", 24) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.imu_accel_sensitivity);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get GUI show IMU setting */
					else if (strncmp((char const *)buf, "GET /getGuiShowImu", 18) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.gui_show_imu);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get GUI invert CIS image setting */
					else if (strncmp((char const *)buf, "GET /getGuiInvertCisImage", 25) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.gui_invert_cis_image);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get screensaver timeout (in seconds) */
					else if (strncmp((char const *)buf, "GET /getScreensaverTimeout", 26) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.screensaver_timeout_sec);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get motion threshold accelerometer */
					else if (strncmp((char const *)buf, "GET /getMotionThresholdAcc", 26) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%.2f", shared_config.motion_threshold_acc);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get motion threshold gyroscope */
					else if (strncmp((char const *)buf, "GET /getMotionThresholdGyro", 27) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%.2f", shared_config.motion_threshold_gyro);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Get firmware version */
					else if (strncmp((char const *)buf, "GET /getFirmwareVersion", 23) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%s", FW_VERSION);
						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Device identity + link state (JSON) */
					else if (strncmp((char const *)buf, "GET /getDeviceInfo", 18) == 0)
					{
						char name[SYS_IDENTITY_NAME_LEN], serial[SYS_IDENTITY_SERIAL_LEN];
						uint8_t mac[6], peer[4];
						sys_identity_name(name);
						sys_identity_serial(serial);
						sys_identity_mac(mac);
						link_getPeerIp(peer);

						char response[400];
						int len = sprintf(response,
								"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nCache-Control: no-cache\r\n\r\n"
								"{"
								"\"name\":\"%s\","
								"\"serial\":\"%s\","
								"\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
								"\"fw\":\"%s\","
								"\"hw\":%d,"
								"\"protocol\":%d,"
								"\"link_port\":%d,"
								"\"bound\":%d,"
								"\"peer\":\"%d.%d.%d.%d\","
								"\"streaming\":%d,"
								"\"lps\":%d"
								"}",
								name, serial,
								mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
								FW_VERSION, (int)HW_REVISION, (int)SLP_VERSION,
								(int)shared_config.network_link_port,
								(int)link_isBound(),
								peer[0], peer[1], peer[2], peer[3],
								(int)udpClient_isStreaming(),
								(int)shared_var.cis_freq);
						netconn_write(conn, response, len, NETCONN_COPY);
					}


					/* Live IMU: one sample per request for the imu.html charts. */
					else if (strncmp((char const *)buf, "GET /getImu", 11) == 0)
					{
						char body[256];
						int bodyLen = sprintf(body,
								"{\"acc\":[%.3f,%.3f,%.3f],\"gyro\":[%.2f,%.2f,%.2f],"
								"\"temp\":%.1f,\"seq\":%u,\"btn\":[%d,%d,%d]}",
								shared_imu.acc[0], shared_imu.acc[1], shared_imu.acc[2],
								shared_imu.gyro[0], shared_imu.gyro[1], shared_imu.gyro[2],
								shared_imu.temp_c, (unsigned)shared_imu.seq,
								shared_var.button_events[0].state == SWITCH_PRESSED,
								shared_var.button_events[1].state == SWITCH_PRESSED,
								shared_var.button_events[2].state == SWITCH_PRESSED);

						/* Polled ~20 times per second: keep the connection like /scan.bin. */
						close = (++served >= HTTP_KEEPALIVE_MAX_REQUESTS);

						char hdr[160];
						int hdrLen = sprintf(hdr,
								"HTTP/1.1 200 OK\r\n"
								"Content-Type: application/json\r\n"
								"Content-Length: %d\r\n"
								"Cache-Control: no-store\r\n"
								"Connection: %s\r\n\r\n",
								bodyLen, close ? "close" : "keep-alive");
						netconn_write(conn, hdr, (size_t)hdrLen, NETCONN_COPY);
						netconn_write(conn, body, (size_t)bodyLen, NETCONN_COPY);
					}

					/* Live scan viewer: the page is served from flash and polls
					 * /scan.bin, accumulating the waterfall in a canvas itself.
					 * The device therefore only ever publishes its latest line. */
					else if (strncmp((char const *)buf, "GET /scan.html", 14) == 0)
					{
						fs_open(&file, "/scan.html");
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}

					/* Scan lines: 24 B preamble, then every line published since the
					 * client's last one. Batching is what makes the viewer usable --
					 * one line per request capped it at the poll rate (25 of the
					 * sensor's 1000 lines per second). */
					else if (strncmp((char const *)buf, "GET /scan.bin", 13) == 0)
					{
						uint8_t dec = 4;
						uint16_t rate = 25;
						uint32_t since = 0;
						const char *arg;

						if ((arg = strstr(buf, "dec=")) != NULL)
						{
							dec = (uint8_t)atoi(arg + 4);
						}
						if ((arg = strstr(buf, "rate=")) != NULL)
						{
							rate = (uint16_t)atoi(arg + 5);
						}
						if ((arg = strstr(buf, "since=")) != NULL)
						{
							since = (uint32_t)strtoul(arg + 6, NULL, 10);
						}
						/* Re-arms the publishing: it stops by itself once the page does. */
						cisScan_requestPreview(dec, rate);

						uint32_t firstId = 0;
						uint16_t pixels = 0, dropped = 0, pubRate = 0;
						const uint16_t lines = cisScan_previewBatch(since, &firstId, &pixels,
						                                            &dropped, &pubRate);
						const uint16_t dpi = (uint16_t)shared_config.cis_dpi;
						const uint16_t lps = (uint16_t)shared_var.cis_freq;
						const uint8_t flags = (uint8_t)((link_isBound() ? 0x01 : 0x00) |
						                               (udpClient_isStreaming() ? 0x02 : 0x00));
						const uint32_t body = 3U * (uint32_t)pixels * (uint32_t)lines;

						/* Little endian: "SCN2", pixels, lines, first id, dpi, scan
						 * rate, published rate, dropped, flags, decimation */
						uint8_t head[24];
						head[0] = 'S'; head[1] = 'C'; head[2] = 'N'; head[3] = '2';
						head[4] = (uint8_t)pixels;          head[5] = (uint8_t)(pixels >> 8);
						head[6] = (uint8_t)lines;           head[7] = (uint8_t)(lines >> 8);
						head[8] = (uint8_t)firstId;         head[9] = (uint8_t)(firstId >> 8);
						head[10] = (uint8_t)(firstId >> 16); head[11] = (uint8_t)(firstId >> 24);
						head[12] = (uint8_t)dpi;            head[13] = (uint8_t)(dpi >> 8);
						head[14] = (uint8_t)lps;            head[15] = (uint8_t)(lps >> 8);
						head[16] = (uint8_t)pubRate;        head[17] = (uint8_t)(pubRate >> 8);
						head[18] = (uint8_t)dropped;        head[19] = (uint8_t)(dropped >> 8);
						head[20] = flags;                   head[21] = dec;
						head[22] = 0;                       head[23] = 0;

						/* Keep the connection for the next batch. The viewer polls tens of
						 * times per second, so it gets a longer burst than the settings
						 * pages: reconnecting mid-scan costs a gap in the image. */
						close = (++served >= HTTP_KEEPALIVE_MAX_SCAN_REQUESTS);

						char hdr[160];
						int hdrLen = sprintf(hdr,
								"HTTP/1.1 200 OK\r\n"
								"Content-Type: application/octet-stream\r\n"
								"Content-Length: %u\r\n"
								"Cache-Control: no-store\r\n"
								"Connection: %s\r\n\r\n",
								(unsigned)(sizeof(head) + body),
								close ? "close" : "keep-alive");
						netconn_write(conn, hdr, (size_t)hdrLen, NETCONN_COPY);
						netconn_write(conn, head, sizeof(head), NETCONN_COPY);
						for (uint16_t i = 0; i < lines; i++)
						{
							const uint8_t *px = cisScan_previewLine(firstId + i);
							if (px == NULL)
							{
								break; /* cannot happen once a line exists; length stays honest */
							}
							if (netconn_write(conn, px, 3U * (size_t)pixels, NETCONN_COPY) != ERR_OK)
							{
								/* Body shorter than Content-Length: this connection can no
								 * longer be reused, close it instead of desyncing it. */
								close = true;
								break;
							}
						}
					}

					/* Read-only file browser (replaces the FTP server). Both
					 * responses say "Connection: close": honour it even when a
					 * previous request on this connection asked for keep-alive. */
					else if (strncmp((char const *)buf, "GET /fs/list", 12) == 0)
					{
						fsbrowse_list(conn, buf);
						close = true;
					}
					else if (strncmp((char const *)buf, "GET /fs/get", 11) == 0)
					{
						fsbrowse_download(conn, buf);
						close = true;
					}

					/* Journal en RAM retenue (bootloader, CM7, CM4) : tirage a la
					 * demande, rien ne circule sans lecteur. */
					else if (strncmp((char const *)buf, "GET /log", 8) == 0 && (buf[8] == '?' || buf[8] == ' '))
					{
						logserve_get(conn, buf);
						close = true;
					}

					/* Anything else: serve the file of that name from the flash file
					 * system (fsdata.c carries each file's HTTP header), 404 otherwise.
					 * Names are matched against the embedded table, so a path cannot
					 * escape it. */
					else
					{
						char path[64];
						const char *req = buf + 4;   /* after "GET " */
						size_t n = 0;
						while (req[n] != '\0' && req[n] != ' ' && req[n] != '?' &&
						       req[n] != '\r' && n < sizeof(path) - 1)
						{
							path[n] = req[n];
							n++;
						}
						path[n] = '\0';
						if (n == 1)   /* bare "/" -- the device opens on its live image */
						{
							strcpy(path, "/scan.html");
						}
						if (fs_open(&file, path) != ERR_OK && fs_open(&file, "/404.html") != ERR_OK)
						{
							/* Nothing to serve at all: answer, never write an unopened file. */
							const char *notFound = "HTTP/1.1 404 Not Found\r\n"
									"Content-Type: text/plain\r\n"
									"Content-Length: 9\r\n\r\n"
									"Not found";
							netconn_write(conn, notFound, strlen(notFound), NETCONN_COPY);
						}
						else
						{
							netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
							fs_close(&file);
						}
					}
				}
				else
				{
					/* Process POST request to set DPI */
					if (strncmp((char const *)buf, "POST /setDPI", 12) == 0)
					{
						char *dpiValue = strstr(buf, "dpi=");  // Point to the first character of the value
						if (dpiValue) dpiValue += 4; /* keep NULL when the key is absent */

						if (dpiValue)
						{
							shared_config.cis_dpi = atoi(dpiValue);
							shared_config.cis_dpi  = shared_config.cis_dpi  < 200 ? 200 : shared_config.cis_dpi  > 200 ? 400 : shared_config.cis_dpi ;
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_config.cis_dpi);
							netconn_write(conn, response, len, NETCONN_COPY);

							// DPI change requires a full system reboot to apply new configuration
							// No need to call cis_reConfigure() as the system will reinitialize on reboot
							reboot = true;
						}
						else
						{
							char *errorResponse = "Error: DPI value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
						}
					}

					/* Redemarre dans le bootloader en mode flasheur reseau (docs/NETBOOT.md).
					 * La reponse part d'abord, le reset suit la fermeture de la connexion. */
					else if (strncmp((char const *)buf, "POST /netboot", 13) == 0)
					{
						const char *resp = "HTTP/1.1 202 Accepted\r\nContent-Type: text/plain\r\n"
						                   "Connection: close\r\n\r\nentering network flash mode";
						netconn_write(conn, resp, strlen(resp), NETCONN_COPY);
						netboot_requested = true;
						close = true;
					}

					/* Process POST request to set hand settings */
					else if (strncmp((char const *)buf, "POST /setHand", 13) == 0)
					{
						char *handValue = strstr(buf, "hand=");
						if (handValue) handValue += 5; /* keep NULL when the key is absent */

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
						char *oversamplingValue = strstr(buf, "oversampling=");
						if (oversamplingValue) oversamplingValue += 13; /* keep NULL when the key is absent */

						if (oversamplingValue)
						{
							shared_config.cis_oversampling = atoi(oversamplingValue);
							shared_config.cis_oversampling  = shared_config.cis_oversampling  < 1 ? 1 : shared_config.cis_oversampling  > 32 ? 32 : shared_config.cis_oversampling ; /* 0 would skip the capture wait in cis_imageProcess() */
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

					/* Point noir : applique a chaud (recomposition de la LUT de rendu), persiste */
					else if (strncmp((char const *)buf, "POST /setBlackPoint", 19) == 0)
					{
						char *bpValue = strstr(buf, "black_point=");
						if (bpValue) bpValue += 12;

						if (bpValue)
						{
							int v = atoi(bpValue);
							if (v < 0) v = 0; else if (v > 200) v = 200;
							shared_config.cis_black_point = (uint8_t)v;
							cis_composeOutputLut();
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nBlack point set to %d", (int)shared_config.cis_black_point);
							netconn_write(conn, response, len, NETCONN_COPY);
						}
						else
						{
							char *errorResponse = "Error: black_point value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
						}
					}

					/* Calibration du point noir : glisser sur le papier noir de reference.
					   Reponse immediate ; la fin se lit sur GET /getBlackPointCalStatus. */
					else if (strncmp((char const *)buf, "POST /startBlackPointCal", 24) == 0)
					{
						cisBlackPointCalState = 1U;
						const char *resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nBlack point calibration started - glide on black paper now";
						netconn_write(conn, resp, strlen(resp), NETCONN_COPY);
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

					/* Process IMU calibration start command */
					else if (strncmp((char const *)buf, "POST /startIMUCalibration", 25) == 0)
					{
					    char *body = strstr(buf, "\r\n\r\n");
					    if (body && strstr(body, "IMU_CAL_START"))
					    {
					        printf("HTTP: IMU calibration requested via web interface\n");

					        // Direct synchronous call - blocks HTTP handler for ~1.2 seconds
					        ICM42688_StatusTypeDef result = icm42688_performCalibration();

					        if (result == ICM42688_OK)
					        {
					            const char *response =
					                "HTTP/1.1 200 OK\r\n"
					                "Content-Type: text/plain\r\n\r\n"
					                "IMU Calibration completed successfully and saved to flash";
					            netconn_write(conn, response, strlen(response), NETCONN_COPY);
					            printf("HTTP: IMU calibration successful\n");
					        }
					        else
					        {
					            const char *response =
					                "HTTP/1.1 500 Internal Server Error\r\n"
					                "Content-Type: text/plain\r\n\r\n"
					                "IMU Calibration failed. Ensure device is stationary during calibration.";
					            netconn_write(conn, response, strlen(response), NETCONN_COPY);
					            printf("HTTP: IMU calibration failed\n");
					        }
					    }
					}

					/* Process POST request to set gyro sensitivity */
					else if (strncmp((char const *)buf, "POST /setGyroSensitivity", 24) == 0)
					{
						char *gyroSensValue = strstr(buf, "gyro_sensitivity=");
						if (gyroSensValue) gyroSensValue += 17; /* keep NULL when the key is absent */

						if (gyroSensValue)
						{
							uint8_t newValue = (uint8_t)atoi(gyroSensValue);
							// Validate range (0x00-0x07 for GyroFS)
							if (newValue <= 0x07)
							{
								shared_config.imu_gyro_sensitivity = newValue;
								file_writeConfig(CONFIG_FILE_PATH, &shared_config);

								printf("HTTP: Gyro sensitivity changed to %u, recalibrating...\n", newValue);

								// Apply new sensitivity and recalibrate immediately
								icm42688_setGyroFS((GyroFS)newValue);

								// Perform gyro calibration with new sensitivity (~200ms)
								if (icm42688_calibrateGyro() == ICM42688_OK)
								{
									// Save the new calibration
									icm42688_saveCalibration(IMU_CALIBRATION_FILE_PATH);
									printf("HTTP: Gyro recalibration successful\n");

									char response[100];
									int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.imu_gyro_sensitivity);
									netconn_write(conn, response, len, NETCONN_COPY);
								}
								else
								{
									printf("HTTP: Gyro recalibration failed\n");
									char *errorResponse = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nGyro recalibration failed";
									netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
								}
							}
							else
							{
								char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nInvalid gyro sensitivity value";
								netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
							}
						}
						else
						{
							char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nGyro sensitivity value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
						}
					}

					/* Process POST request to set accel sensitivity */
					else if (strncmp((char const *)buf, "POST /setAccelSensitivity", 25) == 0)
					{
						char *accelSensValue = strstr(buf, "accel_sensitivity=");
						if (accelSensValue) accelSensValue += 18; /* keep NULL when the key is absent */

						if (accelSensValue)
						{
							uint8_t newValue = (uint8_t)atoi(accelSensValue);
							// Validate range (0x00-0x03 for AccelFS)
							if (newValue <= 0x03)
							{
								shared_config.imu_accel_sensitivity = newValue;
								file_writeConfig(CONFIG_FILE_PATH, &shared_config);

								printf("HTTP: Accel sensitivity changed to %u, performing full IMU recalibration...\n", newValue);

								// Apply new sensitivity and perform full IMU calibration (~1.2s)
								// This recalibrates both gyro and accel with the new accel sensitivity
								ICM42688_StatusTypeDef result = icm42688_performCalibration();

								if (result == ICM42688_OK)
								{
									printf("HTTP: IMU recalibration successful\n");

									char response[100];
									int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.imu_accel_sensitivity);
									netconn_write(conn, response, len, NETCONN_COPY);
								}
								else
								{
									printf("HTTP: IMU recalibration failed\n");
									char *errorResponse = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nIMU recalibration failed";
									netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
								}
							}
							else
							{
								char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nInvalid accel sensitivity value";
								netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
							}
						}
						else
						{
							char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nAccel sensitivity value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
						}
					}

					/* Process POST request to set GUI show IMU */
					else if (strncmp((char const *)buf, "POST /setGuiShowImu", 19) == 0)
					{
						char *value = strstr(buf, "gui_show_imu=");
						if (value) value += 13; /* keep NULL when the key is absent */
						if (value)
						{
							uint8_t newValue = (uint8_t)atoi(value);
							shared_config.gui_show_imu = (newValue > 0) ? 1 : 0;
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.gui_show_imu);
							netconn_write(conn, response, len, NETCONN_COPY);
						}
						else
						{
							char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nValue not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
						}
					}

					/* Process POST request to set GUI invert CIS image */
					else if (strncmp((char const *)buf, "POST /setGuiInvertCisImage", 26) == 0)
					{
						char *value = strstr(buf, "gui_invert_cis_image=");
						if (value) value += 21; /* keep NULL when the key is absent */
						if (value)
						{
							uint8_t newValue = (uint8_t)atoi(value);
							shared_config.gui_invert_cis_image = (newValue > 0) ? 1 : 0;
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.gui_invert_cis_image);
							netconn_write(conn, response, len, NETCONN_COPY);
						}
						else
						{
							char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nValue not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
						}
					}

					/* Process POST request to set screensaver timeout (in seconds) */
					else if (strncmp((char const *)buf, "POST /setScreensaverTimeout", 27) == 0)
					{
						char *value = strstr(buf, "screensaver_timeout=");
						if (value) value += 20; /* keep NULL when the key is absent */
						if (value)
						{
							uint16_t newValue = (uint16_t)atoi(value);
							// Validate range (1-1000 seconds)
							if (newValue >= MIN_SCREENSAVER_TIMEOUT_SEC && newValue <= MAX_SCREENSAVER_TIMEOUT_SEC)
							{
								shared_config.screensaver_timeout_sec = newValue;
								file_writeConfig(CONFIG_FILE_PATH, &shared_config);

								char response[100];
								int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%u", (unsigned int)shared_config.screensaver_timeout_sec);
								netconn_write(conn, response, len, NETCONN_COPY);
							}
							else
							{
								char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nValue out of range (1-1000)";
								netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
							}
						}
						else
						{
							char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nValue not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
						}
					}

					/* Process POST request to set motion threshold accelerometer */
					else if (strncmp((char const *)buf, "POST /setMotionThresholdAcc", 27) == 0)
					{
						char *value = strstr(buf, "motion_threshold_acc=");
						if (value) value += 21; /* keep NULL when the key is absent */
						if (value)
						{
							float newValue = strtof(value, NULL);
							// Validate range (0.01-1.0 g)
							if (newValue >= MIN_MOTION_THRESHOLD_ACC && newValue <= MAX_MOTION_THRESHOLD_ACC)
							{
								shared_config.motion_threshold_acc = newValue;
								file_writeConfig(CONFIG_FILE_PATH, &shared_config);

								char response[100];
								int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%.2f", shared_config.motion_threshold_acc);
								netconn_write(conn, response, len, NETCONN_COPY);
							}
							else
							{
								char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nValue out of range (0.01-1.0)";
								netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
							}
						}
						else
						{
							char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nValue not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
						}
					}

					/* Process POST request to set motion threshold gyroscope */
					else if (strncmp((char const *)buf, "POST /setMotionThresholdGyro", 28) == 0)
					{
						char *value = strstr(buf, "motion_threshold_gyro=");
						if (value) value += 22; /* keep NULL when the key is absent */
						if (value)
						{
							float newValue = strtof(value, NULL);
							// Validate range (0.5-10.0 dps)
							if (newValue >= MIN_MOTION_THRESHOLD_GYRO && newValue <= MAX_MOTION_THRESHOLD_GYRO)
							{
								shared_config.motion_threshold_gyro = newValue;
								file_writeConfig(CONFIG_FILE_PATH, &shared_config);

								char response[100];
								int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%.2f", shared_config.motion_threshold_gyro);
								netconn_write(conn, response, len, NETCONN_COPY);
							}
							else
							{
								char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nValue out of range (0.5-10.0)";
								netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
							}
						}
						else
						{
							char *errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nValue not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_COPY);
						}
					}


					/* Handler for updating network settings */
					else if (strncmp((char const *)buf, "POST /updateNetworkConfig", 25) == 0)
					{
						char *data = strstr((char *)buf, "\r\n\r\n") + 4; // Assuming data starts after the header
						if (data)
						{
							int ip[4], mask[4], gw[4], dest_ip[4], udp_port;
							int link_port = (int)shared_config.network_link_port;
							int stream_when_unbound = (int)shared_config.stream_when_unbound;

							// Parsing POST data
							sscanf(data, "ip=%d.%d.%d.%d&mask=%d.%d.%d.%d&gateway=%d.%d.%d.%d&dest_ip=%d.%d.%d.%d&udp_port=%d&link_port=%d&stream_when_unbound=%d",
									&ip[0], &ip[1], &ip[2], &ip[3],
									&mask[0], &mask[1], &mask[2], &mask[3],
									&gw[0], &gw[1], &gw[2], &gw[3],
									&dest_ip[0], &dest_ip[1], &dest_ip[2], &dest_ip[3],
									&udp_port,
									&link_port,
									&stream_when_unbound);
							if (link_port < 1 || link_port > 65535) link_port = (int)shared_config.network_link_port;
							if (udp_port < 1 || udp_port > 65535) udp_port = (int)shared_config.network_udp_port;

							// Updating shared configuration
							for (int i = 0; i < 4; i++)
							{
								shared_config.network_ip[i] = ip[i];
								shared_config.network_netmask[i] = mask[i];
								shared_config.network_gw[i] = gw[i];
								shared_config.network_dest_ip[i] = dest_ip[i];
							}
							shared_config.network_udp_port = (uint16_t)udp_port;
							shared_config.network_link_port = (uint16_t)link_port;
							shared_config.stream_when_unbound = (stream_when_unbound > 0) ? 1U : 0U;

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

						    len = sprintf(response, "HTTP/1.1 302 Found\r\nLocation: http://%s/network.html\r\n\r\n", newIP);
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
								netconn_write(conn, response, strlen(response), NETCONN_COPY);
							}
							else
							{
								response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nFactory reset done";
								netconn_write(conn, response, strlen(response), NETCONN_COPY);

								// Close connection immediately to prevent any further network activity
								netconn_close(conn);

								// Perform system reset
								System_SafeReset();

								// Mark that we've handled the reboot (should never reach here)
								reboot = false;
							}
						}
					}

					/* firmware update */
					int ret = fwupdate_multipart_state_machine(conn, buf, buflen);
					if (ret == FWUPDATE_STATUS_NONE)
					{
						/* ignore */
					}
					else if (ret == FWUPDATE_STATUS_INPROGRESS)
					{
						/* Don't close the connection! */
						close = false;
					}
					else
					{
						/* Some result, we should close the connection now. */
						close = true;

						if (ret == FWUPDATE_STATUS_DONE)
						{
							reboot = fwupdate_finish(conn);
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
		if (close)
		{
			/* Action requires us to close the connection now instead of
	        blocking on the next netconn_recv. */
			netconn_close(conn);
			break;
		}
	} /* while netconn_recv */
#ifdef HTTP_SERVER_DEBUG
	printf("===== http_server_serve close\n");
#endif

	if (netboot_requested)
	{
		netboot_requested = false;
		netconn_close(conn);
		printf("Entering network flash mode at next reset\n");
		boot_mailbox_request_netboot((const uint8_t *)shared_config.network_ip,
		                             (const uint8_t *)shared_config.network_netmask);
		osDelay(150); /* laisse partir la reponse et la fermeture TCP */
		System_SafeReset();
	}

	if (reboot)
	{
		netconn_close(conn);
		printf("Rebooting in 3 seconds...\n");
		// Delay before reboot to allow Ethernet PHY to stabilize
		// This prevents network timeout issues after factory reset
		osDelay(3000);
		System_SafeReset();
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
#if LWIP_SO_RCVTIMEO
            /* Never block forever on a client that sends nothing (see HTTP_RECV_TIMEOUT_MS). */
            netconn_set_recvtimeout(newconn, HTTP_RECV_TIMEOUT_MS);
#endif
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
	if (xTaskCreate(http_thread, "http_thread", 8192, NULL, osPriorityAboveNormal, &http_ThreadHandle) == pdPASS)
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
