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

#ifndef ECNT_XMODEM_H
#define ECNT_XMODEM_H

#include <stddef.h>
#include <stdint.h>

/* XMODEM control characters */
#define XMODEM_SOH			0x01
#define XMODEM_STX			0x02
#define XMODEM_EOT			0x04
#define XMODEM_ACK			0x06
#define XMODEM_NAK			0x15
#define XMODEM_CAN			0x18
#define XMODEM_CTRLZ			0x1a
#define XMODEM_CRC_REQ			'C'

/*
 * Error codes returned by xmodem_receive(). Note that they are all negative,
 * so a simple "if (ret)" check is enough to detect a failure.
 */
#define XMODEM_ERR_REMOTE_CANCEL	(-1)
#define XMODEM_ERR_OUT_OF_SYNC		(-2)
#define XMODEM_ERR_RETRY_EXCEEDED	(-3)
#define XMODEM_ERR_OUT_OF_MEMORY	(-4)

/*
 * Stream I/O hooks provided by the caller.
 *
 * getc() must be non-blocking: it returns the next received character, or a
 * negative value when the receive FIFO is empty.
 */
struct xmodem_io {
	int (*getc)(void);
	void (*putc)(int ch);
};

/*
 * Receive a file through XMODEM and store it at @dest.
 *
 * @io		Stream I/O hooks.
 * @dest	Destination address.
 * @max_size	Maximum number of bytes that may be written to @dest.
 * @received	On success, receives the number of bytes written to @dest.
 *
 * Returns 0 on success, or one of the XMODEM_ERR_* codes on failure.
 */
int xmodem_receive(const struct xmodem_io *io, uintptr_t dest,
		   size_t max_size, size_t *received);

#endif /* ECNT_XMODEM_H */
