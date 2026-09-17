/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef LPDDR4_16BIT_SANITY_H
#define LPDDR4_16BIT_SANITY_H

#include <errno.h>
#include <inttypes.h>
#include <lpddr4_if.h>

static inline uint32_t ti_lpddr4_intr_ctlint_sf(const ti_lpddr4_privatedata *pd,
						 const ti_lpddr4_intr_ctlinterrupt intr,
						 const bool *irqstatus)
{
	uint32_t ret = 0;

	if (pd == NULL) {
		return (uint32_t)EINVAL;
	} else if (irqstatus == NULL) {
		return (uint32_t)EINVAL;
	} else if ((uint32_t)intr > (uint32_t)LPDDR4_INTR_LOR_BITS) {
		return (uint32_t)EINVAL;
	}

	return ret;
}

static inline uint32_t ti_lpddr4_intr_ack_ctlint_sf(const ti_lpddr4_privatedata *pd,
						     const ti_lpddr4_intr_ctlinterrupt intr)
{
	uint32_t ret = 0;

	if (pd == NULL) {
		return (uint32_t)EINVAL;
	} else if ((uint32_t)intr > (uint32_t)LPDDR4_INTR_LOR_BITS) {
		return (uint32_t)EINVAL;
	}

	return ret;
}

static inline uint32_t ti_lpddr4_intr_phyint_sf(const ti_lpddr4_privatedata *pd,
						 const ti_lpddr4_intr_phyindepinterrupt intr,
						 const bool *irqstatus)
{
	uint32_t ret = 0;

	if (pd == NULL) {
		return (uint32_t)EINVAL;
	} else if (irqstatus == NULL) {
		return (uint32_t)EINVAL;
	} else if ((uint32_t)intr > (uint32_t)LPDDR4_INTR_PHY_INDEP_ANY_VALID_BIT) {
		return (uint32_t)EINVAL;
	}

	return ret;
}

static inline uint32_t ti_lpddr4_intr_ack_phyint_sf(const ti_lpddr4_privatedata *pd,
						     const ti_lpddr4_intr_phyindepinterrupt intr)
{
	uint32_t ret = 0;

	if (pd == NULL) {
		return (uint32_t)EINVAL;
	} else if ((uint32_t)intr > (uint32_t)LPDDR4_INTR_PHY_INDEP_ANY_VALID_BIT) {
		return (uint32_t)EINVAL;
	}

	return ret;
}

#endif  /* LPDDR4_16BIT_SANITY_H */
