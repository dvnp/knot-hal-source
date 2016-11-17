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

#include "include/comm.h"
#include "include/nrf24.h"
#include "include/time.h"
#include "phy_driver.h"
#include "phy_driver_nrf24.h"
#include "nrf24l01_ll.h"

#define _MIN(a, b)		((a) < (b) ? (a) : (b))

/* TODO: Get this values from config file */
static const struct nrf24_mac addr_gw = {
					.address.uint64 = 0xDEADBEEF12345678};
static uint8_t channel_data = 20;
static uint8_t channel_mgmt= 10;

#define DATA_SIZE 128

/* Structure to save broadcast context */
struct nrf24_mgmt {
	uint8_t buffer_rx[DATA_SIZE];
	size_t len_rx;
	uint8_t buffer_tx[DATA_SIZE];
	size_t len_tx;
};

static struct nrf24_mgmt mgmt = {.len_rx = 0, .len_tx = 0};

/* Structure to save peers context */
struct nrf24_data {
	int8_t pipe;
	uint8_t buffer_rx[DATA_SIZE];
	size_t len_rx;
	uint8_t buffer_tx[DATA_SIZE];
	size_t len_tx;
	size_t offset_rx;
	uint8_t seqnumber_rx;
	uint8_t seqnumber_tx;
};

#ifndef ARDUINO	/* If gateway then 5 peers */
static struct nrf24_data peers[5] = {
	{.pipe = -1, .len_rx = 0, .len_tx = 0, .offset_rx = 0, .seqnumber_rx = 0, .seqnumber_tx = 0},
	{.pipe = -1, .len_rx = 0, .len_tx = 0, .offset_rx = 0, .seqnumber_rx = 0, .seqnumber_tx = 0},
	{.pipe = -1, .len_rx = 0, .len_tx = 0, .offset_rx = 0, .seqnumber_rx = 0, .seqnumber_tx = 0},
	{.pipe = -1, .len_rx = 0, .len_tx = 0, .offset_rx = 0, .seqnumber_rx = 0, .seqnumber_tx = 0},
	{.pipe = -1, .len_rx = 0, .len_tx = 0, .offset_rx = 0, .seqnumber_rx = 0, .seqnumber_tx = 0}
};
#else	/* If thing then 1 peer */
static struct nrf24_data peers[1] = {
	{.pipe = -1, .len_rx = 0, .len_tx = 0, .offset_rx = 0},
};
#endif

/* ARRAY SIZE */
#define CONNECTION_COUNTER	((int) (sizeof(peers) \
				 / sizeof(peers[0])))

/* Access Address for each pipe */
static uint8_t aa_pipes[6][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
	{0x35, 0x96, 0xB6, 0xC1, 0x6B},
	{0x77, 0x96, 0xB6, 0xC1, 0x6B},
	{0xD3, 0x96, 0xB6, 0xC1, 0x6B},
	{0xE7, 0x96, 0xB6, 0xC1, 0x6B},
	{0xF0, 0x96, 0xB6, 0xC1, 0x6B}
};

/* Global to save driver index */
static int driverIndex = -1;

/* Local functions */
static inline int alloc_pipe(void)
{
	uint8_t i;

	for (i = 0; i < CONNECTION_COUNTER; i++) {
		if (peers[i].pipe == -1) {
			/* one peer for pipe*/
			peers[i].pipe = i+1;
			return peers[i].pipe;
		}
	}

	/* No free pipe */
	return -1;
}


static inline int read_mgmt(int spi_fd)
{
	ssize_t ilen;
	struct nrf24_io_pack p;
	struct nrf24_ll_mgmt_pdu *ipdu = (struct nrf24_ll_mgmt_pdu *)p.payload;

	/* Read from management pipe */
	p.pipe = 0;

	/* Read data */
	ilen = phy_read(spi_fd, &p, NRF24_MTU);
	if (ilen < 0)
		return -EAGAIN;

	/* If already has something in rx buffer */
	if (mgmt.len_rx != 0)
		return -EBUSY;

	switch(ipdu->type) {
	/* If is a presente type */
	case NRF24_PDU_TYPE_PRESENCE:
	{
		/* Event header structure */
		struct mgmt_nrf24_header *evt = (struct mgmt_nrf24_header*) mgmt.buffer_rx;
		/* Event presence structure */
		struct mgmt_evt_nrf24_bcast_presence *evt_presence =
				(struct mgmt_evt_nrf24_bcast_presence *)evt->payload;
		/* Mac address structure */
		struct nrf24_mac *mac = (struct nrf24_mac *)ipdu->payload;

		/* Header type is a broadcast presence */
		evt->opcode = MGMT_EVT_NRF24_BCAST_PRESENCE;
		evt->index = 0;
		/* Copy source address */
		evt_presence->src.address.uint64 = mac->address.uint64;

		mgmt.len_rx = sizeof(struct nrf24_mac) + sizeof(struct mgmt_nrf24_header);
	}
		break;
	/* If is a connect request type */
	case NRF24_PDU_TYPE_CONNECT_REQ:
	{
		/* Event header structure */
		struct mgmt_nrf24_header *evt = (struct mgmt_nrf24_header*) mgmt.buffer_rx;
		/* Event connect structure */
		struct mgmt_evt_nrf24_connected *evt_connect =
				(struct mgmt_evt_nrf24_connected *)evt->payload;
		/* Link layer connect structure */
		struct nrf24_ll_mgmt_connect *connect =
				(struct nrf24_ll_mgmt_connect *) ipdu->payload;

		/* Header type is a connect request type */
		evt->opcode = MGMT_EVT_NRF24_CONNECTED;
		evt->index = 0;
		/* Copy src and dst address*/
		evt_connect->src.address.uint64 = connect->src_addr.address.uint64;
		evt_connect->dst.address.uint64 = connect->dst_addr.address.uint64;
		/* Copy channel */
		evt_connect->channel = connect ->channel;
		/* Copy access address */
		memcpy(evt_connect->aa, connect->aa, sizeof(aa_pipes[0]));

		mgmt.len_rx = sizeof(struct mgmt_nrf24_header) + sizeof(struct mgmt_evt_nrf24_connected);
	}

	default:
		return -EINVAL;
	}

	/* Returns the amount of bytes read */
	return ilen;
}

static inline int read_data(int spi_fd, int sockfd)
{
	ssize_t ilen;
	size_t plen;
	uint8_t lid;
	struct nrf24_io_pack p;
	const struct nrf24_ll_data_pdu *ipdu = (void *)p.payload;

	p.pipe = sockfd;

	do {

		/*
		 * Reads the data,
		 * on success, the number of bytes read is returned
		 */
		ilen = phy_read(spi_fd, &p, NRF24_MTU);

		if (ilen < 0)
			return -EAGAIN;	/* Try again */

		if (peers[sockfd-1].seqnumber_rx != ipdu->nseq)
			return -EILSEQ; /* Illegal byte sequence */

		if (peers[sockfd-1].seqnumber_rx == 0)
			peers[sockfd-1].offset_rx = 0;

		/* Payloag length = input length - header size */
		plen = ilen - DATA_HDR_SIZE;
		lid = ipdu->lid;

		if (lid == NRF24_PDU_LID_DATA_FRAG && plen < NRF24_PW_MSG_SIZE)
			return -EBADMSG; /* Not a data message */

		/* Reads no more than DATA_SIZE bytes */
		if (peers[sockfd-1].offset_rx + plen > DATA_SIZE)
			plen = DATA_SIZE - peers[sockfd-1].offset_rx;

		memcpy(peers[sockfd-1].buffer_rx + peers[sockfd-1].offset_rx, ipdu->payload, plen);
		peers[sockfd-1].offset_rx += plen;
		peers[sockfd-1].seqnumber_rx++;

	} while (lid != NRF24_PDU_LID_DATA_END && peers[sockfd-1].offset_rx < DATA_SIZE);

	/* If the complete msg is received, resets the sequence number */
	peers[sockfd-1].seqnumber_rx = 0;
	peers[sockfd-1].len_rx = peers[sockfd-1].offset_rx;

	/* Returns de number of bytes received */
	return peers[sockfd-1].offset_rx;
}

static inline int send_data(int spi_fd, int sockfd)
{
	int err;
	struct nrf24_io_pack p;
	struct nrf24_ll_data_pdu *opdu = (void *)p.payload;
	size_t plen, left;

	/* If has nothing to send, returns EBUSY */
	if (peers[sockfd-1].len_tx == 0)
		return -EAGAIN;

	/* If len is larger than the maximum message size */
	if (peers[sockfd-1].len_tx > NRF24_MAX_MSG_SIZE)
		return -EINVAL;

	/* Set pipe to be sent */
	p.pipe = sockfd;

	left = peers[sockfd-1].len_tx;
	while (left) {

		/* Delay to avoid sending all packets too fast */
		hal_delay_us(512);
		/*
		 * If left is larger than the NRF24_PW_MSG_SIZE,
		 * payload length = NRF24_PW_MSG_SIZE,
		 * if not, payload length = left
		 */
		plen = _MIN(left, NRF24_PW_MSG_SIZE);

		/*
		 * If left is larger than the NRF24_PW_MSG_SIZE,
		 * it means that the packet is fragmented,
		 * if not, it means that it is the last packet.
		 */
		opdu->lid = (left > NRF24_PW_MSG_SIZE) ?
			NRF24_PDU_LID_DATA_FRAG : NRF24_PDU_LID_DATA_END;

		/* Packet sequence number */
		opdu->nseq = peers[sockfd-1].seqnumber_tx;

		/* Offset = len - left */
		memcpy(opdu->payload, peers[sockfd-1].buffer_tx + (peers[sockfd-1].len_tx - left), plen);

		/* Send packet */
		err = phy_write(spi_fd, &p, plen + DATA_HDR_SIZE);
		if (err < 0)
			return err;

		left -= plen;
		peers[sockfd-1].seqnumber_tx++;
	}

	err = peers[sockfd-1].len_tx;

	/* Reset len_tx */
	peers[sockfd-1].len_tx = 0;

	return err;
}

static inline int send_mgmt(int spi_fd)
{
	int err;
	struct nrf24_io_pack p;

	/* If already has something to send */
	if (mgmt.len_tx != 0)
		return -EBUSY;

	/* Set pipe to be sent */
	p.pipe = 0;
	/* Copy buffer_tx to payload */
	memcpy(p.payload, mgmt.buffer_tx, mgmt.len_tx);

	err = phy_write(spi_fd, &p, mgmt.len_tx);
	if (err < 0)
		return err;

	/* Reset len_tx */
	mgmt.len_tx = 0;

	return err;
}

static inline int running()
{

	static int state = 0;
	static unsigned long start;
	static int sockIndex = 0;

	switch(state) {

	case 0:
		/* Set channel to management channel */
		phy_ioctl(driverIndex, NRF24_CMD_SET_CHANNEL, &channel_mgmt);
		/* Start timeout */
		start = hal_time_ms();
		/* Go to next state */
		state = 1;
		break;

	case 1:
		/* Check if 10ms timeout occurred */
		if (hal_timeout(hal_time_ms(), start, 10) > 0)
			state = 2;

		/* Read management packets until timeout occurs */
		read_mgmt(driverIndex);
		break;

	case 2:
		/* Set channel to data channel */
		phy_ioctl(driverIndex, NRF24_CMD_SET_CHANNEL, &channel_data);
		/* Start timeout */
		start = hal_time_ms();
		/* Go to next state */
		state = 3;
		break;

	case 3:
		/* Check if 60ms timeout occurred */
		if (hal_timeout(hal_time_ms(), start, 60) > 0)
			state = 0;

		if (sockIndex == CONNECTION_COUNTER)
			sockIndex = 0;

		/* Check if pipe is allocated */
		if (peers[sockIndex].pipe != -1)
			read_data(driverIndex, sockIndex);

		sockIndex++;
		break;

	}

	return -ENOSYS;
}

int hal_comm_init(const char *pathname)
{
	/* If driver not opened */
	if (driverIndex != -1)
		return -EPERM;

	/* Open driver and returns the driver index */
	driverIndex = phy_open(pathname);
	if (driverIndex < 0)
		return driverIndex;

	return 0;
}

int hal_comm_deinit(void)
{
	int err;

	/* If try to close driver with no driver open */
	if (driverIndex == -1)
		return -EPERM;

	/* Close driver */
	err = phy_close(driverIndex);
	if (err < 0)
		return err;

	/* Dereferencing driverIndex */
	driverIndex = -1;

	return err;
}

int hal_comm_socket(int domain, int protocol)
{
	int retval;
	struct addr_pipe ap;

	/* If domain is not NRF24 */
	if (domain != HAL_COMM_PF_NRF24)
		return -EPERM;

	/* If not initialized */
	if (driverIndex == -1)
		return -EPERM;	/* Operation not permitted */

	switch (protocol) {

	case HAL_COMM_PROTO_MGMT:
		/* If Management, disable ACK and returns 0 */
		ap.ack = false;
		retval = 0;
		break;

	case HAL_COMM_PROTO_RAW:
		/*
		 * If raw data, enable ACK
		 * and returns an available pipe
		 * from 1 to 5
		 */
		retval = alloc_pipe();
		/* If not pipe available */
		if (retval < 0)
			return -EUSERS; /* Returns too many users */

		ap.ack = true;
		break;

	default:
		return -EINVAL; /* Invalid argument */
	}

	ap.pipe = retval;
	memcpy(ap.aa, aa_pipes[retval], sizeof(aa_pipes[retval]));

	/* Open pipe */
	phy_ioctl(driverIndex, NRF24_CMD_SET_PIPE, &ap);

	return retval;
}

int hal_comm_close(int sockfd)
{
	if (driverIndex == -1)
		return -EPERM;

	/* Pipe 0 is not closed because ACK arrives in this pipe */
	if (sockfd >= 1 && sockfd <= 5) {
		/* Free pipe */
		peers[sockfd-1].pipe = -1;
		phy_ioctl(driverIndex, NRF24_CMD_RESET_PIPE, &sockfd);
	}

	return 0;
}

ssize_t hal_comm_read(int sockfd, void *buffer, size_t count)
{
	size_t length = 0;

	/* TODO: Run background procedures here */

	if (sockfd < 0 || sockfd > 5 || count == 0)
		return -EINVAL;

	/* If management */
	if (sockfd == 0) {
		/* If has something to read */
		if (mgmt.len_rx != 0) {
			/*
			 * If the amount of bytes available
			 * to be read is greather than count
			 * then read count bytes
			 */
			length = mgmt.len_rx > count ? count : mgmt.len_rx;
			/* Copy rx buffer */
			memcpy(buffer, mgmt.buffer_rx, length);

			/* Reset rx len */
			mgmt.len_rx = 0;
		} else /* Return -EAGAIN has nothing to be read */
			return -EAGAIN;

	} else if (peers[sockfd-1].len_rx != 0) {
		/*
		 * If the amount of bytes available
		 * to be read is greather than count
		 * then read count bytes
		 */
		length = peers[sockfd-1].len_rx > count ?
				 count : peers[sockfd-1].len_rx;
		/* Copy rx buffer */
		memcpy(buffer, peers[sockfd-1].buffer_rx, length);
		/* Reset rx len */
		peers[sockfd-1].len_rx = 0;
	} else
		return -EAGAIN;

	/* Returns the amount of bytes read */
	return length;
}


ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count)
{

	/* TODO: Run background procedures */

	if (sockfd < 1 || sockfd > 5 || count == 0 || count > DATA_SIZE)
		return -EINVAL;

	/* If already has something to write then returns busy */
	if (peers[sockfd-1].len_tx != 0)
		return -EBUSY;

	/* Copy data to be write in tx buffer */
	memcpy(peers[sockfd-1].buffer_tx, buffer, count);
	peers[sockfd-1].len_tx = count;

	return count;
}
int hal_comm_listen(int sockfd)
{

	return -ENOSYS;
}

int hal_comm_accept(int sockfd, uint64_t *addr)
{

	uint8_t datagram[NRF24_MTU];
	struct nrf24_ll_mgmt_pdu *ipdu = (struct nrf24_ll_mgmt_pdu *) datagram;
	struct nrf24_ll_mgmt_connect *payload =
			(struct nrf24_ll_mgmt_connect *) ipdu->payload;
	size_t len;

	/* Read connect_request from pipe broadcast */
	len = mgmt.len_rx;
	if (len == 0)
		return -EAGAIN;

	/* Get the response */
	memcpy(datagram, mgmt.buffer_rx, len);
	/* Reset rx len */
	mgmt.len_rx = 0;
	/* If this packet is not connect request */
	if (ipdu->type != NRF24_PDU_TYPE_CONNECT_REQ)
		return -EINVAL;
	/* If this packet is not for me*/
	if (payload->dst_addr.address.uint64 != *addr)
		return -EINVAL;

	return 0;
}


int hal_comm_connect(int sockfd, uint64_t *addr)
{

	uint8_t datagram[NRF24_MTU];
	struct nrf24_ll_mgmt_pdu *opdu = (struct nrf24_ll_mgmt_pdu *)datagram;
	struct nrf24_ll_mgmt_connect *payload =
				(struct nrf24_ll_mgmt_connect *) opdu->payload;
	size_t len;

	/* If already has something to write then returns busy */
	if (mgmt.len_tx != 0)
		return -EBUSY;

	opdu->type = NRF24_PDU_TYPE_CONNECT_REQ;

	payload->src_addr = addr_gw;
	payload->dst_addr.address.uint64 = *addr;
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

	/* Copy data to be write in tx buffer for BROADCAST*/
	memcpy(mgmt.buffer_tx, datagram, len);
	mgmt.len_tx = len;

	return 0;
}
