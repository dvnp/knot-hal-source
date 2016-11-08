/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

/*
 * Build instructions while a Makefile is not available:
 * gcc $(pkg-config --libs --cflags glib-2.0) -Iinclude -Itools \
 * tools/rpiecho.c -o tools/rpiechod
 */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <glib.h>
#include "nrf24l01.h"

#define NRF24L01_NO_PIPE		0b111
static char *opt_mode = "server";

static GMainLoop *main_loop;

int cli_fd = -1;
int quit = 0;
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
static void sig_term(int sig)
{
	quit = 1;
	nrf24l01_deinit(spi_fd);
	g_main_loop_quit(main_loop);
}


static gboolean timeout_watch_client(gpointer user_data)
{

	int err;

	char buffer[] = {"input"};
	err = -1;

	// /* Puts the radio in TX mode  enabling Acknowledgment */
	// nrf24l01_set_ptx(0, true);

	// /* Puts the radio in TX mode  enabling Acknowledgment */
	// nrf24l01_set_ptx(1, true);

	nrf24l01_set_ptx(spi_fd, 3);
	/* Transmits the data */
	nrf24l01_ptx_data(spi_fd, &buffer, sizeof(buffer));

	/* Waits for ACK */
	err = nrf24l01_ptx_wait_datasent(spi_fd);

	if (err == 0)
		printf("Buffer sent: %s\nsizeof=%ld\n", buffer, sizeof(buffer));
	else
		printf("Error sending message\n");

	return TRUE;
}


static void timeout_watch_server()
{
	uint8_t buffer[200];
	int pipe;
	int length;

	while(!quit){

	/* If the pipe available */
	for (pipe=nrf24l01_prx_pipe_available(spi_fd); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available(spi_fd)) {
		/* Copy data to buffer */
		length = nrf24l01_prx_data(spi_fd, &buffer, sizeof(buffer));
		if(length > 0){
			printf("Message received in pipe %d\n",pipe);
			printf("%s\n", buffer);
			printf("\n");
		}
	}

	}
}

static GOptionEntry options[] = {
	{ "mode", 'm', 0, G_OPTION_ARG_STRING, &opt_mode,
					"mode", "Operation mode: server or client" },
	{ NULL },
};

static int radio_init(void)
{
	printf("call spi init\n");
	spi_fd = nrf24l01_init("/dev/spidev0.0");
	printf("spi_fd is %d\n", spi_fd);
	nrf24l01_set_channel(spi_fd,10);
	nrf24l01_set_standby(spi_fd);
	nrf24l01_open_pipe(spi_fd, 0, aa_pipes[0], false);
	nrf24l01_open_pipe(spi_fd, 1, aa_pipes[1], true);
	nrf24l01_open_pipe(spi_fd, 2, aa_pipes[2], true);
	nrf24l01_open_pipe(spi_fd, 3, aa_pipes[3], true);
	nrf24l01_set_prx(spi_fd, aa_pipes[0]);

	return 0;
}

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	int err, timeout_id=0;

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	// cli_fd = driver->probe();

	/*TO DO: GET PATH CORRECT */
	// driver->open("");

	err = radio_init();
	if (err < 0) {
		g_main_loop_unref(main_loop);
		return -1;
	}
	g_option_context_free(context);

	main_loop = g_main_loop_new(NULL, FALSE);

	printf("RPi nRF24L01 Radio test spi\n");

	if (strcmp(opt_mode, "server") == 0){
		timeout_watch_server();

	}else{
		// timeout_id = g_timeout_add_seconds(1, timeout_watch_client, NULL);
		timeout_id = g_timeout_add_seconds(1, timeout_watch_client, NULL);
	}

	printf("%d\n", timeout_id);

	g_main_loop_run(main_loop);

	// g_source_remove(timeout_id);

	g_main_loop_unref(main_loop);

	return 0;
}
