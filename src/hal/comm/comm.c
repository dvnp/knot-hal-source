/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include "include/comm.h"

static int current_socket = 1;

int hal_comm_open(const char *pathname)
{
	return 0;
}

int hal_comm_socket(int domain, int protocol)
{
	int socket;

	/* Protocol is set to broadcast */
	if (protocol == HAL_COMM_NRF24PROTO_MGMT)
		socket = 0;
	/* The protocol is set to communicate to a specific endpoint */
	else
		socket = current_socket;

	return socket;
}

void hal_comm_close(int sockfd)
{
}

int hal_comm_listen(int sockfd)
{
	return 0;
}

int hal_comm_accept(int sockfd, uint64_t *addr)
{
	return 0;
}
