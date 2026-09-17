// SPDX-License-Identifier: BSD-3-Clause
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#include <errno.h>

#include <common/bl_common.h>
#include <common/debug.h>

#include "cps_drv_lpddr4.h"
#include "lpddr4.h"
#include "lpddr4_if.h"
#include "lpddr4_structs_if.h"

static uint32_t lpddr4_pollctlirq(const ti_lpddr4_privatedata *pd,
				  ti_lpddr4_intr_ctlinterrupt irqbit,
				  uint32_t delay)
{
	uint32_t result = 0U;
	uint32_t timeout = 0U;
	bool irqstatus = false;

	do {
		if (++timeout == delay) {
			result = (uint32_t)EIO;
			break;
		}
		result = ti_lpddr4_checkctlinterrupt(pd, irqbit, &irqstatus);
	} while ((irqstatus == false) && (result == 0U));

	return result;
}

static uint32_t lpddr4_pollphyindepirq(const ti_lpddr4_privatedata *pd,
				       ti_lpddr4_intr_phyindepinterrupt irqbit,
				       uint32_t delay)
{
	uint32_t result = 0U;
	uint32_t timeout = 0U;
	bool irqstatus = false;

	do {
		if (++timeout == delay) {
			result = (uint32_t)EIO;
			break;
		}
		result = ti_lpddr4_checkphyindepinterrupt(pd, irqbit, &irqstatus);
	} while ((irqstatus == false) && (result == 0U));

	return result;
}

static uint32_t lpddr4_pollandackirq(const ti_lpddr4_privatedata *pd)
{
	uint32_t result = 0U;
	lpddr4_ctlregs *ctlregbase = pd->ctlbase;

	result = lpddr4_pollphyindepirq(pd, LPDDR4_INTR_PHY_INDEP_INIT_DONE_BIT,
					TI_LPDDR4_CUSTOM_TIMEOUT_DELAY);
	if (result != 0U) {
		ERROR("lpddr4: init timeout PI=0x%x CTL=0x%x\n",
		       ctlregbase->TI_LPDDR4__PI_INT_STATUS__REG,
		       ctlregbase->DENALI_CTL[342]);
		return result;
	}

	result = ti_lpddr4_ackphyindepinterrupt(pd, LPDDR4_INTR_PHY_INDEP_INIT_DONE_BIT);
	if (result != 0U) {
		return result;
	}

	result = lpddr4_pollctlirq(pd, LPDDR4_INTR_MC_INIT_DONE, TI_LPDDR4_CUSTOM_TIMEOUT_DELAY);
	if (result != 0U) {
		ERROR("lpddr4: init timeout MC=0x%x CTL=0x%x\n",
		       ctlregbase->TI_LPDDR4__INT_STATUS_MASTER__REG,
		       ctlregbase->DENALI_CTL[342]);
		return result;
	}

	result = ti_lpddr4_ackctlinterrupt(pd, LPDDR4_INTR_MC_INIT_DONE);

	return result;
}

static uint32_t lpddr4_startsequencecontroller(const ti_lpddr4_privatedata *pd)
{
	uint32_t result = 0U;
	uint32_t regval = 0U;
	ti_lpddr4_infotype infotype;
	lpddr4_ctlregs *ctlregbase = pd->ctlbase;

	regval = CPS_FLD_SET(TI_LPDDR4__PI_START__FLD,
			     ctlregbase->TI_LPDDR4__PI_START__REG);
	ctlregbase->TI_LPDDR4__PI_START__REG = regval;

	regval = CPS_FLD_SET(TI_LPDDR4__START__FLD,
			     ctlregbase->TI_LPDDR4__START__REG);
	ctlregbase->TI_LPDDR4__START__REG = regval;

	if (pd->infohandler != (lpddr4_infocallback)NULL) {
		infotype = LPDDR4_DRV_SOC_PLL_UPDATE;
		pd->infohandler(pd, infotype);
	}

	result = lpddr4_pollandackirq(pd);

	return result;
}


uint32_t ti_lpddr4_probe(const ti_lpddr4_config *config, uint16_t *configsize)
{
	if ((configsize == NULL) || (config == NULL)) {
		return (uint32_t)EINVAL;
	}

	*configsize = (uint16_t)(sizeof(ti_lpddr4_privatedata));
	return 0U;
}

uint32_t ti_lpddr4_init(ti_lpddr4_privatedata *pd, const ti_lpddr4_config *cfg)
{
	if ((pd == NULL) || (cfg == NULL)) {
		return (uint32_t)EINVAL;
	}

	pd->ctlbase = cfg->ctlbase;
	pd->infohandler = (lpddr4_infocallback)cfg->infohandler;
	pd->ctlinterrupthandler = (lpddr4_ctlcallback)cfg->ctlinterrupthandler;
	pd->phyindepinterrupthandler = (lpddr4_phyindepcallback)cfg->phyindepinterrupthandler;
	return 0U;
}

uint32_t ti_lpddr4_start(const ti_lpddr4_privatedata *pd)
{
	uint32_t result = 0U;

	if (pd == NULL) {
		return (uint32_t)EINVAL;
	}

	ti_lpddr4_enable_pi_initiator(pd);

	result = lpddr4_startsequencecontroller(pd);

	return result;
}

uint32_t ti_lpddr4_readreg(const ti_lpddr4_privatedata *pd, ti_lpddr4_regblock cpp,
			   uint32_t regoffset, uint32_t *regvalue)
{
	if ((pd == NULL) || (regvalue == NULL)) {
		return (uint32_t)EINVAL;
	} else if ((cpp != LPDDR4_CTL_REGS) &&
			(cpp != LPDDR4_PHY_REGS) &&
			(cpp != LPDDR4_PHY_INDEP_REGS)) {
		return (uint32_t)EINVAL;
	}

	lpddr4_ctlregs *ctlregbase = pd->ctlbase;

	switch (cpp) {
	case LPDDR4_CTL_REGS:
		if (regoffset >= TI_LPDDR4_INTR_CTL_REG_COUNT) {
			return (uint32_t)EINVAL;
		}
		*regvalue = ctlregbase->DENALI_CTL[regoffset];
		break;
	case LPDDR4_PHY_REGS:
		if (regoffset >= TI_LPDDR4_INTR_PHY_REG_COUNT) {
			return (uint32_t)EINVAL;
		}
		*regvalue = ctlregbase->DENALI_PHY_0[regoffset];
		break;
	case LPDDR4_PHY_INDEP_REGS:
		if (regoffset >= TI_LPDDR4_INTR_PHY_INDEP_REG_COUNT) {
			return (uint32_t)EINVAL;
		}
		*regvalue = ctlregbase->DENALI_PI[regoffset];
		break;
	default:
		break;
	}

	return 0U;
}

uint32_t ti_lpddr4_writereg(const ti_lpddr4_privatedata *pd, ti_lpddr4_regblock cpp,
			    uint32_t regoffset, uint32_t regvalue)
{
	if (pd == NULL) {
		return (uint32_t)EINVAL;
	} else if ((cpp != LPDDR4_CTL_REGS) &&
			(cpp != LPDDR4_PHY_REGS) &&
			(cpp != LPDDR4_PHY_INDEP_REGS)) {
		return (uint32_t)EINVAL;
	}

	lpddr4_ctlregs *ctlregbase = pd->ctlbase;

	switch (cpp) {
	case LPDDR4_CTL_REGS:
		if (regoffset >= TI_LPDDR4_INTR_CTL_REG_COUNT) {
			return (uint32_t)EINVAL;
		}
		ctlregbase->DENALI_CTL[regoffset] = regvalue;
		break;
	case LPDDR4_PHY_REGS:
		if (regoffset >= TI_LPDDR4_INTR_PHY_REG_COUNT) {
			return (uint32_t)EINVAL;
		}
		ctlregbase->DENALI_PHY_0[regoffset] = regvalue;
		break;
	case LPDDR4_PHY_INDEP_REGS:
		if (regoffset >= TI_LPDDR4_INTR_PHY_INDEP_REG_COUNT) {
			return (uint32_t)EINVAL;
		}
		ctlregbase->DENALI_PI[regoffset] = regvalue;
		break;
	default:
		break;
	}

	return 0U;
}

uint32_t ti_lpddr4_writectlconfigex(const ti_lpddr4_privatedata *pd,
				    const uint32_t regvalues[], uint16_t regcount)
{
	uint32_t result = 0U;
	uint32_t aindex;

	if ((pd == NULL) || (regvalues == (uint32_t *)NULL)) {
		return (uint32_t)EINVAL;
	}

	for (aindex = 0; aindex < regcount; aindex++) {
		result = ti_lpddr4_writereg(pd, LPDDR4_CTL_REGS, aindex,
					    regvalues[aindex]);
		if (result != 0U) {
			return result;
		}
	}

	return result;
}

uint32_t ti_lpddr4_writephyindepconfigex(const ti_lpddr4_privatedata *pd,
					 const uint32_t regvalues[], uint16_t regcount)
{
	uint32_t result = 0U;
	uint32_t aindex;

	if ((pd == NULL) || (regvalues == (uint32_t *)NULL)) {
		return (uint32_t)EINVAL;
	}

	for (aindex = 0; aindex < regcount; aindex++) {
		result = ti_lpddr4_writereg(pd, LPDDR4_PHY_INDEP_REGS, aindex,
					    regvalues[aindex]);
		if (result != 0U) {
			return result;
		}
	}

	return result;
}

uint32_t ti_lpddr4_writephyconfigex(const ti_lpddr4_privatedata *pd,
				    const uint32_t regvalues[], uint16_t regcount)
{
	uint32_t result = 0U;
	uint32_t aindex;

	if ((pd == NULL) || (regvalues == (uint32_t *)NULL)) {
		return (uint32_t)EINVAL;
	}

	for (aindex = 0; aindex < regcount; aindex++) {
		result = ti_lpddr4_writereg(pd, LPDDR4_PHY_REGS, aindex,
					    regvalues[aindex]);
		if (result != 0U) {
			return result;
		}
	}

	return result;
}

uint32_t ti_lpddr4_checkphyindepinterrupt(const ti_lpddr4_privatedata *pd,
					  ti_lpddr4_intr_phyindepinterrupt intr,
					  bool *irqstatus)
{
	uint32_t result = 0;
	uint32_t phyindepirqstatus = 0;

	result = ti_lpddr4_intr_phyint_sf(pd, intr, irqstatus);
	if ((result == 0U) && ((uint32_t)intr < TI_WORD_SHIFT)) {
		lpddr4_ctlregs *ctlregbase = pd->ctlbase;

		phyindepirqstatus = ctlregbase->TI_LPDDR4__PI_INT_STATUS__REG;
		*irqstatus = (((phyindepirqstatus >> (uint32_t)intr) & TI_LPDDR4_BIT_MASK) > 0U);
	}
	return result;
}

uint32_t ti_lpddr4_ackphyindepinterrupt(const ti_lpddr4_privatedata *pd,
					ti_lpddr4_intr_phyindepinterrupt intr)
{
	uint32_t result = 0U;
	uint32_t regval = 0U;

	result = ti_lpddr4_intr_ack_phyint_sf(pd, intr);
	if ((result == 0U) && ((uint32_t)intr < TI_WORD_SHIFT)) {
		lpddr4_ctlregs *ctlregbase = pd->ctlbase;

		regval = (TI_LPDDR4_BIT_MASK << (uint32_t)intr);
		ctlregbase->TI_LPDDR4__PI_INT_ACK__REG = regval;
	}

	return result;
}
