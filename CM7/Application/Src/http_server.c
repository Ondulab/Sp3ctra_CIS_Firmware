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
#include "http_server.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"

#include "string.h"
#include "stdio.h"
#include "stdbool.h"
#include "cmsis_os.h"

#include "ff.h" // FATFS include
#include "diskio.h" // DiskIO include

#include "file_manager.h"

#include "usart.h"

#include "crc.h"

#include "shared.h"

TaskHandle_t http_ThreadHandle = NULL;

typedef enum {
	FWUPDATE_STATE_HEADER,
	FWUPDATE_STATE_DOWNLOAD_START,
	FWUPDATE_STATE_DOWNLOAD_STREAM,

} fwupdate_state_t;

typedef struct {

	fwupdate_state_t state;

	int content_length;
	int file_length;
	int accum_length;

	/* Data stream accumulation buffer to meet 8-byte size-alignment constraint with v1.3.0 .sfb file update */
	uint8_t accum_buf[8];
	uint8_t accum_buf_len;

} fwupdate_t;

static fwupdate_t fwupdate;

#define CONTENT_LENGTH_TAG      "Content-Length:"
//#define OCTET_STREAM_TAG      "application/octet-stream\r\n\r\n"
#define OCTET_STREAM_TAG 			"Content-Type: image/png\r\n\r\n"
#define EMPTY_LINE_TAG          "\r\n\r\n"

#define FWUPDATE_STATUS_ERROR          -1
#define FWUPDATE_STATUS_NONE            0
#define FWUPDATE_STATUS_INPROGRESS      1
#define FWUPDATE_STATUS_DONE            2

void print_buf(const uint8_t* buf, int len)
{
	for (int i = 0; i < len; i++) {
		uint8_t c = buf[i];
		if (c == 0) {
			char b[] = { 'n','u','l' };
			HAL_UART_Transmit(&huart1, (uint8_t*) & b, sizeof(b), 100);
		} else {
			HAL_UART_Transmit(&huart1, &c, 1, 100);
		}
	}
	uint8_t n = '\n';
	HAL_UART_Transmit(&huart1, &n, 1, 100);
}

static const char* fwupdate_state_str(fwupdate_state_t state)
{
	switch (state) {
	case FWUPDATE_STATE_HEADER: return "HEADER";
	case FWUPDATE_STATE_DOWNLOAD_START: return "OCTET_START";
	case FWUPDATE_STATE_DOWNLOAD_STREAM: return "OCTET_STREAM";
	default:
		return "<unknown>";
	}

}
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

typedef enum
{
	FW_UPDATE_Stage_IDLE,
	FW_UPDATE_Stage_UPDATE,
	FW_UPDATE_Stage_VERIFIED
}  FW_UPDATE_Stage;

// Structure pour gérer l'état et le statut de la mise à jour
typedef struct
{
	bool completed;
	bool error;
	FW_UPDATE_StatusCode code;
	FW_UPDATE_Stage stage;
	uint32_t accumBytes;
	uint32_t totalBytes;
	uint32_t has_been_initialized;
} FW_UPDATE_Status;

/**
 * @brief  Secure Engine Error definition
 */
typedef enum
{
	SE_ERROR = 0U,
	FU_SUCCESS = !SE_ERROR
} FU_ErrorStatus;

#define FU_SUCCESS 1
#define SE_ERROR -1

// Définitions pour la gestion des étapes de la mise à jour
#define FW_UPDATE_Stage_NOT_STARTED 0
#define FW_UPDATE_Stage_IN_PROGRESS 1
#define FW_UPDATE_Stage_VERIFIED 2

FW_UPDATE_Status staticStatus = {0};

/**
 * Initialiser le statut de mise à jour
 */
void FW_UPDATE_InitStatus(FW_UPDATE_Status *status) {
	status->completed = staticStatus.completed;
	status->error = staticStatus.error;
	status->code = staticStatus.code;
	status->stage = staticStatus.stage;
	status->accumBytes = staticStatus.accumBytes;
	status->totalBytes = staticStatus.totalBytes;
}

static FIL file;

#define FW_UPDATE_RebootDelay_NEXT            ((uint32_t)1)

FU_ErrorStatus FW_UPDATE_Data(FW_UPDATE_Status *status, const uint8_t *data, uint32_t data_len)
{
	FRESULT fr;
	UINT bytes_written;

	if (!staticStatus.has_been_initialized)
	{
		fr = f_open(&file, "update.bin", FA_WRITE | FA_OPEN_APPEND);
		if (fr != FR_OK) {
			status->code = fr;
			return SE_ERROR;
		}
		staticStatus.has_been_initialized = 1;
		status->stage = FW_UPDATE_Stage_IN_PROGRESS;
	}

	fr = f_write(&file, data, data_len, &bytes_written);
	if (fr != FR_OK || bytes_written != data_len) {
		status->code = fr;
		return SE_ERROR;
	}

	staticStatus.accumBytes += data_len;
	FW_UPDATE_InitStatus(status);
	printf("@ fwupdate accumBytes=%d\n", (int)staticStatus.accumBytes);

	if (staticStatus.accumBytes >= status->totalBytes) {
		f_close(&file);
		staticStatus.has_been_initialized = 0;
		status->stage = FW_UPDATE_Stage_VERIFIED;
	}

	return FU_SUCCESS;
}

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

void FW_UPDATE_Finish()
{

}

FU_ErrorStatus FW_UPDATE_Init(FW_UPDATE_Status *status, const uint32_t totalLength)
{
	if (status == NULL)
	{
		return SE_ERROR;
	}

	status->totalBytes = totalLength;
	status->accumBytes = 0;
	status->stage = FW_UPDATE_Stage_UPDATE;
	status->completed = false;
	status->error = false;

	staticStatus.completed = status->completed;
	staticStatus.error = status->error;
	staticStatus.code = status->code;
	staticStatus.stage = status->stage;
	staticStatus.accumBytes = status->accumBytes;
	staticStatus.totalBytes = status->totalBytes;

	return FU_SUCCESS;
}

static int fwupdate_multipart_state_machine(struct netconn* conn, char* buf, u16_t buflen)
{
	int ret = FWUPDATE_STATUS_NONE;

	char* buf_start = buf;
	char* buf_end = buf + buflen; /* points to byte AFTER end of buffer! */

	/* We advance the buffer pointer as parts are consumed.  Keep processing
    until buffer pointer gets nulled either as a result of not finding what we want
    or explicitly on error condition.
	 */
	while (buf && buf < buf_end) {

		printf("@ fwupdate buf_start=%p, buf=%p buf_end=%p state=%s\n", buf_start, buf, buf_end, fwupdate_state_str(fwupdate.state));

		switch (fwupdate.state)
		{
		case FWUPDATE_STATE_HEADER:
			/* Look for POST header */
			if ((buflen >= 12) && (strncmp(buf, "POST /upload", 12) == 0)) {
				ret = FWUPDATE_STATUS_ERROR; /* Error until we go to in progress */
				/* This buffer must have the complete POST header */
				printf("@ fwupdate -     Scanning HEADER\n");
				/* Diagnostics */
				print_buf((uint8_t*)buf, buflen);
				buf = strstr(buf, CONTENT_LENGTH_TAG);
				if (buf) {
					buf += strlen(CONTENT_LENGTH_TAG);
					fwupdate.content_length = atoi(buf);
					/* sanity check  */
					if (fwupdate.content_length > 0) {
						/* Having obtained content length, need to next find the
                        end of the header, denoted by an empty line.
                        This is where the content-length starts from. */
						buf = strstr(buf, EMPTY_LINE_TAG);
						if (buf) {
							buf += strlen(EMPTY_LINE_TAG);
							/* To get the length of the file, have to subtract all bytes up to and including
                            application/octet-stream\r\n\r\n"
							 */
							/* advance the buffer reference point to here */
							buf_start = buf;
							/* Got what we need/expect and must wait for the "octet-stream" content-type in
                             the next multipart header packet. */
							fwupdate.state = FWUPDATE_STATE_DOWNLOAD_START;
							ret = FWUPDATE_STATUS_INPROGRESS;

							printf("@ fwupdate - Have content len=%d\n", fwupdate.content_length);
						}
					}
					else {
						buf = 0;
					}
				}
			}
			else {
				buf = 0;
			}
			break;
		case FWUPDATE_STATE_DOWNLOAD_START:

			ret = FWUPDATE_STATUS_ERROR;
			fwupdate.state = FWUPDATE_STATE_HEADER;

			/* Look for the start of the firmware update file */
			buf = strstr(buf, OCTET_STREAM_TAG);
			if (buf) {
				buf += strlen(OCTET_STREAM_TAG);
				int multipart_length = buf - buf_start;
				/* Strictly speaking this computed file length is incorrect - it is a bit too large as it includes
                the ending multipart boundary tag of few dozen chars. This doesn't impact the update process because the
                patching engine manages byte counters internally. This is just for diagnostic purposes and sanity check.
				 */
				fwupdate.file_length = fwupdate.content_length - multipart_length;
				printf("@ fwupdate content len=%d multipart len=%d file len=%d\n",
						fwupdate.content_length, multipart_length, fwupdate.file_length);
				/* Now can init the update */
				FW_UPDATE_Status FW_UPDATE_status;
				FU_ErrorStatus status = FW_UPDATE_Init(&FW_UPDATE_status, fwupdate.file_length);
				printf("@ fwupdate INIT %d\n", status);
				//fwupdate_PrintStatus(&FW_UPDATE_status);
				if (status == FU_SUCCESS)
				{
					/* Now need to consume rest of buffer as the file */
					fwupdate.state = FWUPDATE_STATE_DOWNLOAD_STREAM;
					fwupdate.accum_length = 0;
					fwupdate.accum_buf_len = 0;
					ret = FWUPDATE_STATUS_INPROGRESS;
				}
				else
				{
					//fwupdate_send_err(conn, fwupdate_GetStatusCodeString(FW_UPDATE_status.code));
				}
			}
			break;

		case FWUPDATE_STATE_DOWNLOAD_STREAM:
		{
			int len = buf_end - buf;
			printf("@ fwupdate len=%d\n", len);

			const uint8_t* input_buf; /* pointer to data we will consume */
			int input_buf_len = 0; /* number of bytes to consume from this input buffer pointer */
			int offset = 0; /* offset into the original buffer */

			ret = FWUPDATE_STATUS_INPROGRESS;

			FW_UPDATE_Status FW_UPDATE_status;
			FW_UPDATE_InitStatus(&FW_UPDATE_status);

			/* One way or another we MUST consume all bytes in the received packet (buf).
			 * Bytes that aren't a multiple of the accumulation buffer size are stored
			 * in the accumulation buffer until next packet.
			 */
			while (len > 0) {

				/* ----------------------------------------------------------------- */
				input_buf = 0;

				if (fwupdate.accum_buf_len > 0 ||
						len < sizeof(fwupdate.accum_buf)) {
					/* We have existing bytes in the buffer or and /or too few bytes to process.
                      Store them in our internal buffer.
					 */
					int avail = sizeof(fwupdate.accum_buf) - fwupdate.accum_buf_len;
					int copy = len > avail ? avail : len;
					memcpy(fwupdate.accum_buf + fwupdate.accum_buf_len, buf + offset, copy);
					fwupdate.accum_buf_len += copy;

					/* consumed these bytes into the front end buffer */
					len -= copy;
					offset += copy;

					printf("@   accum %d bytes\n", copy);
				}

				/* If the front end buffer is full, use it */
				if (fwupdate.accum_buf_len == sizeof(fwupdate.accum_buf)) {
					printf("@   use accum_buf\n");
					/* We now have all required minimum bytes in the front end buffer.Process them */
					input_buf = fwupdate.accum_buf;
					fwupdate.accum_buf_len = 0;
					input_buf_len = sizeof(fwupdate.accum_buf);
				}
				// otherwise use a block directly from original buffer if available
				else if (len >= sizeof(fwupdate.accum_buf)) {
					// No waiting bytes and we have enough to process another block(s)
					input_buf = (const uint8_t*)buf + offset;
					printf("@   use buf at %d\n", offset);

					/* We will try to consume all bytes that are a multiple of the accumulation buffer.
                    The rest will get consumed into the accumulation buffer and held for the next packet received.
					 */
					input_buf_len = (len / sizeof(fwupdate.accum_buf)) * sizeof(fwupdate.accum_buf);
					len -= input_buf_len;
					offset += input_buf_len;
				}
				/* ----------------------------------------------------------------- */

				/* Only if there is at least one accum_buf number of bytes available */
				if (input_buf) {

					//print_hex_buf((const uint8_t*)input_buf, input_buf_len);

					FW_UPDATE_InitStatus(&FW_UPDATE_status);
					FU_ErrorStatus status = FW_UPDATE_Data(&FW_UPDATE_status, input_buf, input_buf_len);
					printf("@ fwupdate DATA len=%d status=%d\n", input_buf_len, status);
					//					fwupdate_PrintStatus(&FW_UPDATE_status);
					if (status == SE_ERROR) {
						ret = FWUPDATE_STATUS_ERROR;
						fwupdate.state = FWUPDATE_STATE_HEADER;
					}
					else {
						fwupdate.accum_length += len;
					}
				}
			}

			/* In any case we have consumed all bytes remaining in this buffer */
			buf = 0;

			if (ret == FWUPDATE_STATUS_INPROGRESS) {
				if (fwupdate.accum_length >= fwupdate.file_length
						|| FW_UPDATE_status.stage == FW_UPDATE_Stage_VERIFIED)
				{
					/* now the thing is, if rebootdelay_IMMEDIATE was selected, the device will have rebooted
                    before we get here.
					 */
					if (FW_UPDATE_status.stage == FW_UPDATE_Stage_VERIFIED) {
						ret = FWUPDATE_STATUS_DONE;
					}
					else {
						ret = FWUPDATE_STATUS_ERROR;
					}

					/* if we have all file bytes, we are done whether it worked or not */
					fwupdate.state = FWUPDATE_STATE_HEADER;
				}
			}

			break;
		}
		}
	} /* while (buf) */

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


	printf("===== http_server_serve recv\n");

	/* Read the data from the port, blocking if nothing yet there.
	   We assume the request (the part we care about) is in one netbuf */
	while ((recv_err = netconn_recv(conn, &inbuf)) == ERR_OK)
	{
		if (netconn_err(conn) == ERR_OK)
		{
			do {
				netbuf_data(inbuf, (void**)&buf, &buflen);
				printf("# Process buffer: %p %d bytes\n", buf, buflen);

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

					/* Process POST request to set DPI */
					else if (strncmp((char const *)buf, "POST /setDPI", 12) == 0)
					{
						char *dpiValue = strstr(buf, "dpi=") + 4;  // Point to the first character of the value

						if (dpiValue) {
							shared_config.cis_dpi = atoi(dpiValue);
							shared_config.cis_dpi  = shared_config.cis_dpi  < 200 ? 200 : shared_config.cis_dpi  > 200 ? 400 : shared_config.cis_dpi ;
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int) shared_config.cis_dpi);
							netconn_write(conn, response, len, NETCONN_COPY);
						} else {
							char *errorResponse = "Error: DPI value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
						}
					}

					/* Get and set oversampling settings */
					else if (strncmp((char const *)buf, "GET /getOversampling", 20) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_config.cis_oversampling);

						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Process POST request to set oversampling */
					else if (strncmp((char const *)buf, "POST /setOversampling", 21) == 0)
					{
						char *oversamplingValue = strstr(buf, "oversampling=") + 13;

						if (oversamplingValue) {
							shared_config.cis_oversampling = atoi(oversamplingValue);
							shared_config.cis_oversampling  = shared_config.cis_oversampling  < 0 ? 0 : shared_config.cis_oversampling  > 32 ? 32 : shared_config.cis_oversampling ;
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOversampling set to %d", (int)shared_config.cis_oversampling);
							netconn_write(conn, response, len, NETCONN_COPY);
						} else {
							char *errorResponse = "Error: Oversampling value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
						}
					}

					/* Get hand settings */
					else if (strncmp((char const *)buf, "GET /getHand", 12) == 0)
					{
						char response[100];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_config.cis_handedness);

						netconn_write(conn, response, len, NETCONN_COPY);
					}

					/* Process POST request to set hand settings */
					else if (strncmp((char const *)buf, "POST /setHand", 13) == 0)
					{
						char *handValue = strstr(buf, "hand=") + 5;

						if (handValue) {
							shared_config.cis_handedness = atoi(handValue);
							shared_config.cis_handedness =  shared_config.cis_handedness < 0 ? 0 :  shared_config.cis_handedness > 1 ? 1 :  shared_config.cis_handedness;
							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int) shared_config.cis_handedness);
							netconn_write(conn, response, len, NETCONN_COPY);
						} else {
							char *errorResponse = "Error: Hand value not found";
							netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
						}
					}

					/* Process calibration start command */
					else if (strncmp((char const *)buf, "POST /startCalibration", 22) == 0)
					{
						char *body = strstr(buf, "\r\n\r\n");
						if (body && strstr(body, "CIS_CAL_START")) {
							shared_var.cis_cal_state = CIS_CAL_REQUESTED;
							const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nCalibration started";
							netconn_write(conn, response, strlen(response), NETCONN_COPY);
						}
					}

					/* Get network settings */
					else if (strncmp((char const *)buf, "GET /getNetworkConfig", 21) == 0)
					{
						char response[300];
						int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
								"{"
								"\"ip\":\"%d.%d.%d.%d\","
								"\"mask\":\"%d.%d.%d.%d\","
								"\"gw\":\"%d.%d.%d.%d\","
								"\"dest_ip\":\"%d.%d.%d.%d\","
								"\"udp_port\":%d,"
								"\"broadcast\":%d"
								"}",
								shared_config.network_ip[0], shared_config.network_ip[1], shared_config.network_ip[2], shared_config.network_ip[3],
								shared_config.network_netmask[0], shared_config.network_netmask[1], shared_config.network_netmask[2], shared_config.network_netmask[3],
								shared_config.network_gw[0], shared_config.network_gw[1], shared_config.network_gw[2], shared_config.network_gw[3],
								shared_config.network_dest_ip[0], shared_config.network_dest_ip[1], shared_config.network_dest_ip[2], shared_config.network_dest_ip[3],
								shared_config.network_udp_port,
								shared_config.network_broadcast);

						netconn_write(conn, response, len, NETCONN_COPY);
					}


					/* Handler for updating network settings */
					else if (strncmp((char const *)buf, "POST /updateNetworkConfig", 25) == 0)
					{
						char *data = strstr((char *)buf, "\r\n\r\n") + 4; // Assuming data starts after the header
						if (data) {
							int ip[4], mask[4], gw[4], dest_ip[4], udp_port, broadcast;

							// Parsing POST data
							sscanf(data, "ip=%d.%d.%d.%d&mask=%d.%d.%d.%d&gateway=%d.%d.%d.%d&dest_ip=%d.%d.%d.%d&udp_port=%d&broadcast=%d",
									&ip[0], &ip[1], &ip[2], &ip[3],
									&mask[0], &mask[1], &mask[2], &mask[3],
									&gw[0], &gw[1], &gw[2], &gw[3],
									&dest_ip[0], &dest_ip[1], &dest_ip[2], &dest_ip[3],
									&udp_port, &broadcast);

							// Updating shared configuration
							for (int i = 0; i < 4; i++) {
								shared_config.network_ip[i] = ip[i];
								shared_config.network_netmask[i] = mask[i];
								shared_config.network_gw[i] = gw[i];
								shared_config.network_dest_ip[i] = dest_ip[i];
							}
							shared_config.network_udp_port = udp_port;
							shared_config.network_broadcast = broadcast;

							file_writeConfig(CONFIG_FILE_PATH, &shared_config);

							char response[100];
							int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nNetwork settings updated.");

							netconn_write(conn, response, len, NETCONN_COPY);
						} else {
							char response[100];
							int len = sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nInvalid request.");
							netconn_write(conn, response, len, NETCONN_COPY);
						}
					}

					/* Send 404 if no route matches */
					else
					{
						fs_open(&file, "/404.html");
						netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
						fs_close(&file);
					}

				}
				else {
					/* Not a handled GET request - process this as a potential multipart form POST */
					int ret = fwupdate_multipart_state_machine(conn, buf, buflen);
					if (ret == FWUPDATE_STATUS_NONE) {
						/* ignore */
					}
					else if (ret == FWUPDATE_STATUS_INPROGRESS) {
						/* Don't close the connection! */
						close = false;
					}
					else {
						/* Some result, we should close the connection now. */
						close = true;

						if (ret == FWUPDATE_STATUS_DONE) {
							/* reboot after we close the connection. */
							reboot = true;
						}
					}
				}
				/* Process all data that may be present in the netbuf */
				printf("# netbuf_next = %d\n", netbuf_next(inbuf));
			} while (netbuf_next(inbuf) >= 0);

			/* Delete the buffer (netconn_recv gives us ownership,
	         so we have to make sure to deallocate the buffer) */
			netbuf_delete(inbuf);
		}
		else {
			printf("# netconn_recv error: %d %d\n", recv_err, netconn_err(conn));
		}
		if (close) {
			/* Action requires us to close the connection now instead of
	        blocking on the next netconn_recv. */
			break;
		}
	} /* while netconn_recv */

	printf("===== http_server_serve close\n");
	/* Close the connection (server closes in HTTP) */
	netconn_close(conn);

	if (reboot) {
		printf("Rebooting in 5\n");
		/* Wait 5 seconds. */
		osDelay(5000);
		NVIC_SystemReset();
	}

}

static void http_thread(void *arg)
{
	printf("----- HTTP THREAD STARTED ------\n");

	struct netconn *conn, *newconn;
	err_t err, accept_err;

	/* Create a new TCP connection handle */
	conn = netconn_new(NETCONN_TCP);
	if (conn != NULL)
	{
		err = netconn_bind(conn, IP_ADDR_ANY, 80);
		if (err == ERR_OK)
		{
			netconn_listen(conn);
			while (1)
			{
				accept_err = netconn_accept(conn, &newconn);
				if (accept_err == ERR_OK)
				{
					http_server(newconn);
					netconn_delete(newconn);
				}
			}
		}
		else
		{
			printf("Bind failed with error: %d\n", err);
		}
	}
	else
	{
		printf("Failed to create new TCP connection handle.\n");
	}
}

void http_serverInit()
{
	printf("----- HTTP INITIALIZATIONS ----\n");
	if (xTaskCreate(http_thread, "http_thread", 4096, NULL, osPriorityNormal, &http_ThreadHandle) == pdPASS)
	{
		printf("http initialisation FU_SUCCESS\n");
	}
	else
	{
		printf("Failed to create http task.\n");
	}
}
