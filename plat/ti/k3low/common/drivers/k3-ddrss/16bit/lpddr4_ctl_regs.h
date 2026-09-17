/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef LPDDR4_CTL_REGS_H
#define LPDDR4_CTL_REGS_H

#include <cdefs.h>

#include "lpddr4_macros.h"

typedef struct __packed ti_lpddr4_ctlregs_s {
	volatile uint32_t DENALI_CTL[423];
	volatile char pad__0[0x1964U];
	volatile uint32_t DENALI_PI[345];
	volatile char pad__1[0x1A9CU];
	volatile uint32_t DENALI_PHY_0[126];
	volatile char pad__2[0x208U];
	volatile uint32_t DENALI_PHY_256[126];
	volatile char pad__3[0x208U];
	volatile uint32_t DENALI_PHY_512[43];
	volatile char pad__4[0x354U];
	volatile uint32_t DENALI_PHY_768[43];
	volatile char pad__5[0x354U];
	volatile uint32_t DENALI_PHY_1024[43];
	volatile char pad__6[0x354U];
	volatile uint32_t DENALI_PHY_1280[126];
} lpddr4_ctlregs;

#endif /* LPDDR4_CTL_REGS_H */
