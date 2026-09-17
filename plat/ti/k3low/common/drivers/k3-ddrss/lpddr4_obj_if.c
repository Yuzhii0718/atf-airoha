// SPDX-License-Identifier: BSD-3-Clause
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#include "lpddr4_obj_if.h"

ti_lpddr4_obj *ti_lpddr4_getinstance(void)
{
	static ti_lpddr4_obj driver = {
		.probe				= ti_lpddr4_probe,
		.init				= ti_lpddr4_init,
		.start				= ti_lpddr4_start,
		.readreg			= ti_lpddr4_readreg,
		.writereg			= ti_lpddr4_writereg,
		.writectlconfigex		= ti_lpddr4_writectlconfigex,
		.writephyconfigex		= ti_lpddr4_writephyconfigex,
		.writephyindepconfigex		= ti_lpddr4_writephyindepconfigex,
		.checkctlinterrupt		= ti_lpddr4_checkctlinterrupt,
		.ackctlinterrupt		= ti_lpddr4_ackctlinterrupt,
		.checkphyindepinterrupt		= ti_lpddr4_checkphyindepinterrupt,
		.ackphyindepinterrupt		= ti_lpddr4_ackphyindepinterrupt,
	};

	return &driver;
}
