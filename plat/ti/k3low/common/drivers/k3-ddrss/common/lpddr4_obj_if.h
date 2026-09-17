/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef LPDDR4_OBJ_IF_H
#define LPDDR4_OBJ_IF_H

#include "lpddr4_if.h"

typedef struct ti_lpddr4_obj_s {
	uint32_t (*probe)(const ti_lpddr4_config *config, uint16_t *configsize);
	uint32_t (*init)(ti_lpddr4_privatedata *pd, const ti_lpddr4_config *cfg);
	uint32_t (*start)(const ti_lpddr4_privatedata *pd);
	uint32_t (*readreg)(const ti_lpddr4_privatedata *pd, ti_lpddr4_regblock cpp,
			    uint32_t regoffset, uint32_t *regvalue);
	uint32_t (*writereg)(const ti_lpddr4_privatedata *pd, ti_lpddr4_regblock cpp,
			     uint32_t regoffset, uint32_t regvalue);
	uint32_t (*writectlconfigex)(const ti_lpddr4_privatedata *pd,
				     const uint32_t regvalues[], uint16_t regcount);
	uint32_t (*writephyconfigex)(const ti_lpddr4_privatedata *pd,
				     const uint32_t regvalues[], uint16_t regcount);
	uint32_t (*writephyindepconfigex)(const ti_lpddr4_privatedata *pd,
					  const uint32_t regvalues[], uint16_t regcount);
	uint32_t (*checkctlinterrupt)(const ti_lpddr4_privatedata *pd,
				      ti_lpddr4_intr_ctlinterrupt intr, bool *irqstatus);
	uint32_t (*ackctlinterrupt)(const ti_lpddr4_privatedata *pd,
				    ti_lpddr4_intr_ctlinterrupt intr);
	uint32_t (*checkphyindepinterrupt)(const ti_lpddr4_privatedata *pd,
					   ti_lpddr4_intr_phyindepinterrupt intr,
					   bool *irqstatus);
	uint32_t (*ackphyindepinterrupt)(const ti_lpddr4_privatedata *pd,
					 ti_lpddr4_intr_phyindepinterrupt intr);
} ti_lpddr4_obj;

ti_lpddr4_obj *ti_lpddr4_getinstance(void);

#endif  /* LPDDR4_OBJ_IF_H */
