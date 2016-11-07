/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

#include "spi.h"
#include "nrf24l01.h"

int32_t count;
int q;
//#define NRF24L01_NO_PIPE		0b111
int spi_fd;
/* Access Address for each pipe */
static uint8_t aa_pipes[6][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
	{0x35, 0x96, 0xB6, 0xC1, 0x6B},
	{0x77, 0x96, 0xB6, 0xC1, 0x6B},
	{0xD3, 0x96, 0xB6, 0xC1, 0x6B},
	{0xE7, 0x96, 0xB6, 0xC1, 0x6B},
	{0xF0, 0x96, 0xB6, 0xC1, 0x6B}
};
static GMainLoop *main_loop;

static void sig_term(int sig)
{
	nrf24l01_deinit(spi_fd);
	q = 1;
	g_main_loop_quit(main_loop);
}

static gboolean timeout_watch(gpointer user_data){

		int ret;
		char buffer[] = {"input"};
		ret = nrf24l01_ptx_data(spi_fd, &buffer, sizeof(buffer));
		ret = nrf24l01_ptx_wait_datasent(spi_fd);
		if (ret <0)
			printf("ERRO!!%d \n\n\n",count);
		if (ret == 0)
			printf("MSG SENT\n");

		usleep(4000);

	return true;
}

int main(int argc, char *argv[])
{
	int timeout_id;
	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);


	// main_loop = g_main_loop_new(NULL, FALSE);

	printf("RPi nRF24L01 Radio test tool 7 m\n");

	printf("init\n");
	spi_fd = nrf24l01_init("/dev/spidev0.0");
	printf("set channel\n");
	nrf24l01_set_channel(spi_fd, 10);
	nrf24l01_set_standby(spi_fd);
	//open pipe 0
	nrf24l01_open_pipe(spi_fd, 0, aa_pipes[0]);
	nrf24l01_open_pipe(spi_fd, 1, aa_pipes[1]);
	nrf24l01_open_pipe(spi_fd, 2, aa_pipes[2]);
	// printf("open pip 1\n");
	// nrf24l01_open_pipe(1, aa_pipes[1], spi_fd);

	// printf("set ptx 1\n");
	// nrf24l01_set_ptx(1, true);
	printf("set ptx 2\n");
	nrf24l01_set_ptx(spi_fd, 2, true);

	main_loop = g_main_loop_new(NULL, FALSE);
	timeout_id = g_timeout_add_seconds(1, timeout_watch, NULL);

	g_main_loop_run(main_loop);

	g_source_remove(timeout_id);

	g_main_loop_unref(main_loop);

	return 0;
}
