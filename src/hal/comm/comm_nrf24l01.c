/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef ARDUINO
#include <avr_errno.h>
#include <avr_unistd.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

#include "nrf24l01.h"
#include "nrf24l01_ll.h"
#include "include/comm.h"
#include "phy_driver.h"
#include "phy_driver_nrf24.h"


int8_t pipes_allocate[] = {0, 0, 0, 0, 0};

int driverIndex = -1;

/* Local functions */
inline int get_free_pipe(void)
{
	uint8_t i;

	for (i = 0; i < sizeof(pipes_allocate); i++) {
		if (pipes_allocate[i] == 0) {
			/* one peer for pipe*/
			pipes_allocate[i] = 1;
			return i+1;
		}
	}

	/* No free pipe */
	return -1;
}

/* TODO: Get this values from config file */
static const uint64_t addr_gw = 0xDEADBEEF12345678;
static const uint8_t channel_data = 20;

static uint8_t aa_pipes[6][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
	{0x35, 0x96, 0xB6, 0xC1, 0x6B},
	{0x77, 0x96, 0xB6, 0xC1, 0x6B},
	{0xD3, 0x96, 0xB6, 0xC1, 0x6B},
	{0xE7, 0x96, 0xB6, 0xC1, 0x6B},
	{0xF0, 0x96, 0xB6, 0xC1, 0x6B}
};

int hal_comm_socket(int domain, int protocol)
{
	int retval;

	switch (protocol) {

	case HAL_COMM_PROTO_MGMT:
		driverIndex = phy_open("NRF0");
		if (driverIndex < 0)
			return driverIndex;

		retval = 0;
		break;

	case HAL_COMM_PROTO_RAW:

		if (driverIndex == -1)
			return -EPERM;	/* Operation not permitted */

		retval = get_free_pipe();
		if (retval < 0)
			return - EUSERS;
		break;

	default:
		retval = -EINVAL;  /* Invalid argument */
		break;
	}

	return retval;
}

void hal_comm_close(int sockfd)
{

	if (sockfd > 1 && sockfd < 5) {
		/* Free pipe */
		pipes_allocate[sockfd-1] = 0;

		/* Close driver */
		phy_close(driverIndex);
	}

}

ssize_t hal_comm_read(int sockfd, void *buffer, size_t count)
{

	return -ENOSYS;
}

ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count)
{

	return -ENOSYS;
}

int hal_comm_listen(int sockfd)
{

	return -ENOSYS;
}

int hal_comm_accept(int sockfd, uint64_t *addr)
{

	return -ENOSYS;
}


int hal_comm_connect(int sockfd, uint64_t *addr)
{

	uint8_t datagram[NRF24_MTU];
	struct nrf24_ll_mgmt_pdu *opdu = (struct nrf24_ll_mgmt_pdu *)datagram;
	struct nrf24_ll_mgmt_connect *payload =
				(struct nrf24_ll_mgmt_connect *) opdu->payload;
	size_t len;
	int err;
	struct nrf24_io_pack packt_drive;
	opdu->type = NRF24_PDU_TYPE_CONNECT_REQ;

	payload->src_addr = addr_gw;
	payload->dst_addr = *addr;
	payload->channel = channel_data;
	/*
	 * Set in payload the addr to be set in client.
	 * sockfd contains the pipe allocated for the client
	 * aa_pipes contains the Access Address for each pipe
	 */
	memcpy(payload->aa, aa_pipes[sockfd],
		sizeof(aa_pipes[sockfd]));

	len = sizeof(struct nrf24_ll_mgmt_connect);
	len += sizeof(struct nrf24_ll_mgmt_pdu);

	/*Send connect_request in broadcast */
	packt_drive.pipe = PIPE_BROADCAST;
	memcpy(packt_drive.payload, datagram, NRF24_MTU);
	err = phy_write(driverIndex, &packt_drive, len);
	if (err < 0)
		return -ECOMM;

	return sockfd;
}
