/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026, Yuzhii0718 <admin@yuzhii0718.eu.org>. All rights reserved.
 *
 * XMODEM recovery mode for the EcoNet/Airoha EN7523 / AN758x platforms.
 *
 * When no valid FIP (BL31 + U-Boot) can be fetched from the configured boot
 * device, BL2 asks the user to upload one over the serial console using the
 * XMODEM protocol and keeps booting from the image just downloaded. This
 * allows rescuing a device whose firmware was erased or corrupted.
 *
 * Ported from the MediaTek APSOC XMODEM recovery (same author), adapted to
 * drive the serial console through the registered console_t instead of raw
 * MMIO, so it works on every SoC without hard-coding a UART base address.
 */

#include <stddef.h>
#include <stdint.h>

#include <common/debug.h>
#include <drivers/console.h>

#include <xmodem.h>

extern int plat_check_header(uint8_t *base);

/*
 * The UART is driven through the console framework: XMODEM needs a
 * non-blocking getc() and the protocol bytes must not be altered by the
 * console (CR/LF translation). A file-static pointer carries the console
 * handle into the xmodem_io callbacks.
 */
static console_t *xmodem_console;

static int xmodem_console_getc(void)
{
	int c = xmodem_console->getc(xmodem_console);

	return (c < 0) ? -1 : (c & 0xff);
}

static void xmodem_console_putc(int ch)
{
	xmodem_console->putc(ch & 0xff, xmodem_console);
}

int ecnt_xmodem_recovery(console_t *console, uintptr_t loadaddr,
			 size_t max_size)
{
	static const struct xmodem_io io = {
		.getc = xmodem_console_getc,
		.putc = xmodem_console_putc,
	};
	size_t received = 0;
	int ret;

	xmodem_console = console;

	console_flush();

	/*
	 * Loop forever: a corrupted or incomplete upload must never brick the
	 * device, so we keep soliciting a new transfer until a valid FIP TOC
	 * header is received.
	 */
	for (;;) {
		NOTICE("Press 'x' to load BL31 + U-Boot FIP via XMODEM\n");

		while (xmodem_console_getc() != 'x')
			;

		ret = xmodem_receive(&io, loadaddr, max_size, &received);
		if (ret == 0 && plat_check_header((uint8_t *)loadaddr) != 0) {
			NOTICE("Received FIP: %zu bytes\n", received);
			return (int)received;
		}

		ERROR("XMODEM FIP is incomplete or has an invalid TOC header (%d)\n",
		      ret);
	}
}
