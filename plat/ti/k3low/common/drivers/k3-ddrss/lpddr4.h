/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef LPDDR4_H
#define LPDDR4_H

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>

#include "lpddr4_16bit_sanity.h"
#include "lpddr4_ctl_regs.h"

#define TI_LPDDR4_BIT_MASK    (0x1U)
#define TI_WORD_SHIFT (32U)
#ifndef TI_LPDDR4_CUSTOM_TIMEOUT_DELAY
#define TI_LPDDR4_CUSTOM_TIMEOUT_DELAY 100000000U
#endif

void ti_lpddr4_enable_pi_initiator(const ti_lpddr4_privatedata *pd);

#endif  /* LPDDR4_H */
