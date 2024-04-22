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
			if (strncmp((char const *)buf, "GET /config.html", 15) == 0) {
				fs_open(&file, "/config.html");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
				fs_close(&file);
			} else if (strncmp((char const *)buf, "GET /img/CISYNTH.png", 15) == 0) {
				fs_open(&file, "/img/CISYNTH.png");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
				fs_close(&file);
			} else if (strncmp((char const *)buf, "GET /buttoncolor=", 17) == 0) {
				colour = buf[17];
			} else if (strncmp((char const *)buf, "GET /getvalue", 13) == 0) {
				char *pagedata = pvPortMalloc(32);
				int len = sprintf(pagedata, "%d", (int)shared_var.cis_freq);
				netconn_write(conn, (const unsigned char*)pagedata, (size_t)len, NETCONN_NOCOPY);
				vPortFree(pagedata);
			} else if (strncmp((char const *)buf, "GET /setoversampling=", 21) == 0) {
				shared_var.cis_oversampling = atoi(&buf[21]);
			} else {
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
