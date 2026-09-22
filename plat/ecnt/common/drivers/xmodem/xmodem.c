/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026, Yuzhii0718 <admin@yuzhii0718.eu.org>. All rights reserved.
 *
 * XMODEM receive-only protocol driver with CRC-16 and 1K (STX) support.
 * The state machine follows the classic XMODEM implementation written by
 * Pascal Stang (Copyright (C) 2006), reworked for TF-A.
 *
 * Ported to the EcoNet/Airoha EN7523 / AN758x platforms.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <drivers/delay_timer.h>

#include <xmodem.h>

#define XMODEM_PKT_SIZE_STD		128
#define XMODEM_PKT_SIZE_1K		1024
#define XMODEM_MAX_PKT_SIZE		XMODEM_PKT_SIZE_1K

/* Timeout used when waiting for a single character / draining the FIFO */
#define XMODEM_TIMEOUT_MS		1000
#define XMODEM_RETRY_LIMIT		16

/*
 * Packet buffer. Kept in .bss on purpose: BL2 runs with a small stack and
 * a 1K packet has to be buffered before it can be validated.
 *
 * Layout: <SOH|STX> <seq> <~seq> <data...> <crc16|checksum>
 */
static uint8_t xmodem_pkt[XMODEM_MAX_PKT_SIZE + 6];

static uint16_t crc_xmodem_update(uint16_t crc, uint8_t data)
{
	int i;

	crc ^= (uint16_t)data << 8;

	for (i = 0; i < 8; i++) {
		if (crc & 0x8000)
			crc = (uint16_t)((crc << 1) ^ 0x1021);
		else
			crc = (uint16_t)(crc << 1);
	}

	return crc;
}

static bool xmodem_pkt_check(bool crc_mode, const uint8_t *data, size_t size)
{
	size_t i;

	if (crc_mode) {
		uint16_t crc = 0;
		uint16_t pkt_crc = ((uint16_t)data[size] << 8) | data[size + 1];

		for (i = 0; i < size; i++)
			crc = crc_xmodem_update(crc, data[i]);

		return crc == pkt_crc;
	} else {
		uint8_t sum = 0;

		for (i = 0; i < size; i++)
			sum += data[i];

		return sum == data[size];
	}
}

/* Read a character, waiting at most @timeout_ms for it to arrive */
static int xmodem_getc_timeout(const struct xmodem_io *io, uint32_t timeout_ms)
{
	unsigned long timeout = (unsigned long)timeout_ms * 1000;
	int ch = io->getc();

	while ((ch < 0) && (timeout > 0)) {
		udelay(1);
		ch = io->getc();
		timeout--;
	}

	return ch;
}

/* Drain everything the remote side is still sending */
static void xmodem_flush(const struct xmodem_io *io)
{
	while (xmodem_getc_timeout(io, XMODEM_TIMEOUT_MS) >= 0)
		;
}

int xmodem_receive(const struct xmodem_io *io, uintptr_t dest,
		   size_t max_size, size_t *received)
{
	uint8_t *buf = (uint8_t *)dest;
	uint8_t seqnum = 1;
	uint8_t response = XMODEM_CRC_REQ;
	bool crc_mode = false;
	size_t total = 0;
	unsigned int retry = XMODEM_RETRY_LIMIT;
	int err = XMODEM_ERR_RETRY_EXCEEDED;
	int ch;

	while (retry > 0) {
		size_t pktsize, trailer, expected, i;

		/* Solicit a connection or the next packet */
		io->putc(response);

		ch = xmodem_getc_timeout(io, XMODEM_TIMEOUT_MS);
		if (ch < 0) {
			/* Timed out, ask again */
			retry--;
			continue;
		}

		switch (ch) {
		case XMODEM_SOH:
			pktsize = XMODEM_PKT_SIZE_STD;
			break;

		case XMODEM_STX:
			pktsize = XMODEM_PKT_SIZE_1K;
			break;

		case XMODEM_EOT:
			xmodem_flush(io);
			io->putc(XMODEM_ACK);
			*received = total;
			return 0;

		case XMODEM_CAN:
			if (xmodem_getc_timeout(io, XMODEM_TIMEOUT_MS) ==
			    XMODEM_CAN) {
				xmodem_flush(io);
				io->putc(XMODEM_ACK);
				return XMODEM_ERR_REMOTE_CANCEL;
			}
			/* Not a real cancel, fall through and retry */
			retry--;
			xmodem_flush(io);
			response = XMODEM_NAK;
			continue;

		default:
			retry--;
			xmodem_flush(io);
			response = XMODEM_NAK;
			continue;
		}

		/* A packet sent in reply to 'C' uses CRC-16 */
		if (response == XMODEM_CRC_REQ)
			crc_mode = true;

		/* <seq> <~seq> <data> <crc16|checksum> */
		trailer = crc_mode ? 2 : 1;
		expected = pktsize + trailer + 2;

		for (i = 0; i < expected; i++) {
			ch = xmodem_getc_timeout(io, XMODEM_TIMEOUT_MS);
			if (ch < 0) {
				retry--;
				xmodem_flush(io);
				response = XMODEM_NAK;
				break;
			}

			xmodem_pkt[i] = (uint8_t)(ch & 0xff);
		}

		/* Packet was truncated, ask for it again */
		if (i < expected)
			continue;

		/* xmodem_pkt[0] = seq, [1] = ~seq, [2..] = data */
		if ((xmodem_pkt[0] != (uint8_t)~xmodem_pkt[1]) ||
		    !xmodem_pkt_check(crc_mode, &xmodem_pkt[2], pktsize)) {
			retry--;
			xmodem_flush(io);
			response = XMODEM_NAK;
			continue;
		}

		if (xmodem_pkt[0] == seqnum) {
			if ((total + pktsize) > max_size) {
				err = XMODEM_ERR_OUT_OF_MEMORY;
				goto out;
			}

			memcpy(&buf[total], &xmodem_pkt[2], pktsize);
			total += pktsize;
			seqnum++;

			/* A good packet resets the retry counter */
			retry = XMODEM_RETRY_LIMIT;
			response = XMODEM_ACK;
		} else if (xmodem_pkt[0] == (uint8_t)(seqnum - 1)) {
			/* Retransmission of the previous packet, drop it */
			response = XMODEM_ACK;
		} else {
			/* We are completely out of sync */
			err = XMODEM_ERR_OUT_OF_SYNC;
			goto out;
		}
	}

out:
	/* Abort the transmission */
	xmodem_flush(io);
	io->putc(XMODEM_CAN);
	io->putc(XMODEM_CAN);
	io->putc(XMODEM_CAN);

	return err;
}
