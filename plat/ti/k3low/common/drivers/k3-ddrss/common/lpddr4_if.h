/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef LPDDR4_IF_H
#define LPDDR4_IF_H

#include <inttypes.h>
#include <stdbool.h>

#include <lpddr4_16bit_if.h>

typedef struct ti_lpddr4_config_s ti_lpddr4_config;
typedef struct ti_lpddr4_privatedata_s ti_lpddr4_privatedata;

typedef enum {
	LPDDR4_CTL_REGS		= 0U,
	LPDDR4_PHY_REGS		= 1U,
	LPDDR4_PHY_INDEP_REGS	= 2U
} ti_lpddr4_regblock;

typedef enum {
	LPDDR4_DRV_NONE				= 0U,
	LPDDR4_DRV_SOC_PLL_UPDATE		= 1U,
	LPDDR4_DRV_SOC_PHY_REDUCED_FREQ		= 2U,
	LPDDR4_DRV_SOC_PHY_DATA_RET_ASSERT	= 3U,
	LPDDR4_DRV_SOC_PHY_DATA_RET_DEASSERT	= 4U
} ti_lpddr4_infotype;

typedef void (*lpddr4_infocallback)(const ti_lpddr4_privatedata *pd, ti_lpddr4_infotype infotype);

typedef void (*lpddr4_ctlcallback)(const ti_lpddr4_privatedata *pd,
				   ti_lpddr4_intr_ctlinterrupt ctlinterrupt,
				   uint8_t chipselect);

typedef void (*lpddr4_phyindepcallback)(const ti_lpddr4_privatedata *pd,
					ti_lpddr4_intr_phyindepinterrupt phyindepinterrupt,
					uint8_t chipselect);

uint32_t ti_lpddr4_probe(const ti_lpddr4_config *config, uint16_t *configsize);

uint32_t ti_lpddr4_init(ti_lpddr4_privatedata *pd, const ti_lpddr4_config *cfg);

uint32_t ti_lpddr4_start(const ti_lpddr4_privatedata *pd);

uint32_t ti_lpddr4_readreg(const ti_lpddr4_privatedata *pd, ti_lpddr4_regblock cpp,
			   uint32_t regoffset, uint32_t *regvalue);

uint32_t ti_lpddr4_writereg(const ti_lpddr4_privatedata *pd, ti_lpddr4_regblock cpp,
			    uint32_t regoffset, uint32_t regvalue);

uint32_t ti_lpddr4_writectlconfigex(const ti_lpddr4_privatedata *pd,
				    const uint32_t regvalues[], uint16_t regcount);

uint32_t ti_lpddr4_writephyconfigex(const ti_lpddr4_privatedata *pd,
				    const uint32_t regvalues[], uint16_t regcount);

uint32_t ti_lpddr4_writephyindepconfigex(const ti_lpddr4_privatedata *pd,
					 const uint32_t regvalues[], uint16_t regcount);

uint32_t ti_lpddr4_checkctlinterrupt(const ti_lpddr4_privatedata *pd,
				     ti_lpddr4_intr_ctlinterrupt intr, bool *irqstatus);

uint32_t ti_lpddr4_ackctlinterrupt(const ti_lpddr4_privatedata *pd,
				   ti_lpddr4_intr_ctlinterrupt intr);

uint32_t ti_lpddr4_checkphyindepinterrupt(const ti_lpddr4_privatedata *pd,
					  ti_lpddr4_intr_phyindepinterrupt intr,
					  bool *irqstatus);

uint32_t ti_lpddr4_ackphyindepinterrupt(const ti_lpddr4_privatedata *pd,
					ti_lpddr4_intr_phyindepinterrupt intr);
#endif  /* LPDDR4_IF_H */
