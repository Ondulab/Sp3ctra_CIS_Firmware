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
#include <http_server.h>
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "string.h"
#include <stdio.h>
#include "cmsis_os.h"

#include "shared.h"

char colour;
int indx = 0;

static void http_server(struct netconn *conn)
{
	struct netbuf *inbuf;
	err_t recv_err;
	char* buf;
	u16_t buflen;
	struct fs_file file;

	/* Read the data from the port, blocking if nothing yet there */
	recv_err = netconn_recv(conn, &inbuf);

	if (recv_err == ERR_OK)
	{
		if (netconn_err(conn) == ERR_OK)
		{
			/* Get the data pointer and length of the data inside a netbuf */
			netbuf_data(inbuf, (void**)&buf, &buflen);

			/* Check for various paths and handle GET requests */
			if (strncmp((char const *)buf, "GET /config.html", 16) == 0)
			{
				fs_open(&file, "/config.html");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
				fs_close(&file);
			}

			else if (strncmp((char const *)buf, "GET /img/CISYNTH.png", 20) == 0)
			{
				fs_open(&file, "/img/CISYNTH.png");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
				fs_close(&file);
			}

			else if (strncmp((char const *)buf, "GET /img/favicon.ico", 20) == 0)
			{
				fs_open(&file, "/img/favicon.ico");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
				fs_close(&file);
			}

			else if (strncmp((char const *)buf, "GET /getFreq", 12) == 0)
			{
			    char response[128];
			    int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_var.cis_freq);
			    netconn_write(conn, response, len, NETCONN_COPY);
			}

			else if (strncmp((char const *)buf, "GET /getDPI", 11) == 0)
			{
			    char response[100];
			    int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_var.cis_dpi);

			    netconn_write(conn, response, len, NETCONN_COPY);
			}

			else if (strncmp((char const *)buf, "POST /setDPI", 12) == 0)
			{
			    // Trouver le début de la valeur après "dpi="
			    char *dpiValue = strstr(buf, "dpi=") + 4;  // Pointer sur le premier caractère de la valeur

			    if (dpiValue) {
			        // Convertir la valeur DPI en entier et mettre à jour
			        shared_var.cis_dpi = atoi(dpiValue);

			        // Envoie une réponse pour confirmer la mise à jour
			        char response[100];
			        int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_var.cis_dpi);
			        netconn_write(conn, response, len, NETCONN_COPY);
			    } else {
			        // Envoie une réponse d'erreur si "dpi=" n'est pas trouvé
			        char *errorResponse = "Error: DPI value not found";
			        netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
			    }
			}

			else if (strncmp((char const *)buf, "GET /getOversampling", 20) == 0)
			{
			    // Prepare a response with the current oversampling value
			    char response[100];
			    int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_var.cis_oversampling);

			    // Send the response
			    netconn_write(conn, response, len, NETCONN_COPY);
			}

			else if (strncmp((char const *)buf, "POST /setOversampling", 21) == 0)
			{
			    // Finding the start of the value after "oversampling="
			    char *oversamplingValue = strstr(buf, "oversampling=") + 13;  // Point to the first character of the value

			    if (oversamplingValue) {
			        // Convert the oversampling value to an integer and update
			        shared_var.cis_oversampling = atoi(oversamplingValue);

			        // Send a response to confirm the update
			        char response[100];
			        int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOversampling set to %d", (int)shared_var.cis_oversampling);
			        netconn_write(conn, response, len, NETCONN_COPY);
			    } else {
			        // Send an error response if "oversampling=" is not found
			        char *errorResponse = "Error: Oversampling value not found";
			        netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
			    }
			}

			else if (strncmp((char const *)buf, "GET /getHand", 12) == 0)
			{
			    char response[100];
			    int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_var.cis_scanDir);

			    netconn_write(conn, response, len, NETCONN_COPY);
			}

			else if (strncmp((char const *)buf, "POST /setHand", 13) == 0)
			{
			    // Trouver le début de la valeur après "hand="
			    char *handValue = strstr(buf, "hand=") + 5;  // Pointer sur le premier caractère de la valeur

			    if (handValue) {
			        // Convertir la valeur Hand en entier et mettre à jour
			        shared_var.cis_scanDir = atoi(handValue);

			        // Envoie une réponse pour confirmer la mise à jour
			        char response[100];
			        int len = sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%d", (int)shared_var.cis_scanDir);
			        netconn_write(conn, response, len, NETCONN_COPY);
			    } else {
			        // Envoie une réponse d'erreur si "hand=" n'est pas trouvé
			        char *errorResponse = "Error: Hand value not found";
			        netconn_write(conn, errorResponse, strlen(errorResponse), NETCONN_NOCOPY);
			    }
			}

			else if (strncmp((char const *)buf, "POST /startCalibration", 22) == 0)
			{
			    // Analyse du corps de la requête pour extraire la valeur
			    char *body = strstr(buf, "\r\n\r\n");
			    if (body && strstr(body, "CIS_CAL_START=0")) {
			        shared_var.cis_cal_state = CIS_CAL_START; // Mettre à jour la variable de calibration
			        const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nCalibration started";
			        netconn_write(conn, response, strlen(response), NETCONN_COPY);
			    }
			}

			else
			{
				// if none match, send 404
				fs_open(&file, "/404.html");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
				fs_close(&file);
			}

		}
	}
	/* Close the connection (server closes in HTTP) */
	netconn_close(conn);

	/* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
	netbuf_delete(inbuf);
}

static void http_thread(void *arg)
{
	struct netconn *conn, *newconn;
	err_t err, accept_err;

	/* Create a new TCP connection handle */
	conn = netconn_new(NETCONN_TCP);

	if (conn!= NULL)
	{
		/* Bind to port 80 (HTTP) with default IP address */
		err = netconn_bind(conn, IP_ADDR_ANY, 80);

		if (err == ERR_OK)
		{
			/* Put the connection into LISTEN state */
			netconn_listen(conn);

			while(1)
			{
				/* accept any incoming connection */
				accept_err = netconn_accept(conn, &newconn);
				if(accept_err == ERR_OK)
				{
					/* serve connection */
					http_server(newconn);

					/* delete connection */
					netconn_delete(newconn);

				}
			}
		}
	}
}

void http_serverInit()
{
	sys_thread_new("http_thread", http_thread, NULL, 2048, osPriorityNormal);
}
