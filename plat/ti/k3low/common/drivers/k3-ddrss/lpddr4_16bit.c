// SPDX-License-Identifier: BSD-3-Clause
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#include <errno.h>

#include <lib/utils_def.h>

#include "cps_drv_lpddr4.h"
#include "lpddr4.h"
#include "lpddr4_ctl_regs.h"
#include "lpddr4_if.h"
#include "lpddr4_structs_if.h"

#define LPDDR4_INT_STATUS_MASTER_MASK GENMASK(31, 31)

struct ctlint_entry {
	uint8_t grp_shift;
	uint8_t int_shift;
};

static const struct ctlint_entry ctlintmap[] = {
	[LPDDR4_INTR_TIMEOUT_ZQ_CAL_INIT]		= { 0,  7  },
	[LPDDR4_INTR_TIMEOUT_ZQ_CALLATCH]		= { 0,  8  },
	[LPDDR4_INTR_TIMEOUT_ZQ_CALSTART]		= { 0,  9  },
	[LPDDR4_INTR_TIMEOUT_MRR_TEMP]			= { 0,  14 },
	[LPDDR4_INTR_TIMEOUT_DQS_OSC_REQ]		= { 0,  15 },
	[LPDDR4_INTR_TIMEOUT_DFI_UPDATE]		= { 0,  16 },
	[LPDDR4_INTR_TIMEOUT_LP_WAKEUP]			= { 0,  17 },
	[LPDDR4_INTR_TIMEOUT_AUTO_REFRESH_MAX]		= { 0,  19 },
	[LPDDR4_INTR_ECC_ERROR]				= { 1,  0  },
	[LPDDR4_INTR_LP_DONE]				= { 2,  0  },
	[LPDDR4_INTR_LP_TIMEOUT]			= { 2,  3  },
	[LPDDR4_INTR_PORT_TIMEOUT]			= { 3,  0  },
	[LPDDR4_INTR_RFIFO_TIMEOUT]			= { 4,  0  },
	[LPDDR4_INTR_TRAINING_ZQ_STATUS]		= { 5,  11 },
	[LPDDR4_INTR_TRAINING_DQS_OSC_DONE]		= { 5,  12 },
	[LPDDR4_INTR_TRAINING_DQS_OSC_UPDATE_DONE]	= { 5,  13 },
	[LPDDR4_INTR_TRAINING_DQS_OSC_OVERFLOW]		= { 5,  14 },
	[LPDDR4_INTR_TRAINING_DQS_OSC_VAR_OUT]		= { 5,  15 },
	[LPDDR4_INTR_USERIF_OUTSIDE_MEM_ACCESS]		= { 6,  0  },
	[LPDDR4_INTR_USERIF_MULTI_OUTSIDE_MEM_ACCESS]	= { 6,  1  },
	[LPDDR4_INTR_USERIF_PORT_CMD_ERROR]		= { 6,  2  },
	[LPDDR4_INTR_USERIF_WRAP]			= { 6,  6  },
	[LPDDR4_INTR_USERIF_INVAL_SETTING]		= { 6,  7  },
	[LPDDR4_INTR_MISC_MRR_TRAFFIC]			= { 7,  3  },
	[LPDDR4_INTR_MISC_SW_REQ_MODE]			= { 7,  4  },
	[LPDDR4_INTR_MISC_CHANGE_TEMP_REFRESH]		= { 7,  5  },
	[LPDDR4_INTR_MISC_TEMP_ALERT]			= { 7,  6  },
	[LPDDR4_INTR_MISC_REFRESH_STATUS]		= { 7,  7  },
	[LPDDR4_INTR_BIST_DONE]				= { 8,  0  },
	[LPDDR4_INTR_CRC]				= { 9,  0  },
	[LPDDR4_INTR_DFI_UPDATE_ERROR]			= { 10, 0  },
	[LPDDR4_INTR_DFI_PHY_ERROR]			= { 10, 1  },
	[LPDDR4_INTR_DFI_BUS_ERROR]			= { 10, 2  },
	[LPDDR4_INTR_DFI_STATE_CHANGE]			= { 10, 3  },
	[LPDDR4_INTR_DFI_DLL_SYNC_DONE]		= { 10, 4  },
	[LPDDR4_INTR_DFI_TIMEOUT]			= { 10, 5  },
	[LPDDR4_INTR_DIMM]				= { 11, 0  },
	[LPDDR4_INTR_FREQ_DFS_REQ_HW_IGNORE]		= { 12, 0  },
	[LPDDR4_INTR_FREQ_DFS_HW_TERMINATE]		= { 12, 1  },
	[LPDDR4_INTR_FREQ_DFS_HW_DONE]			= { 12, 2  },
	[LPDDR4_INTR_FREQ_DFS_REQ_SW_IGNORE]		= { 12, 3  },
	[LPDDR4_INTR_FREQ_DFS_SW_TERMINATE]		= { 12, 4  },
	[LPDDR4_INTR_FREQ_DFS_SW_DONE]			= { 12, 5  },
	[LPDDR4_INTR_INIT_MEM_RESET_DONE]		= { 13, 0  },
	[LPDDR4_INTR_MC_INIT_DONE]			= { 13, 1  },
	[LPDDR4_INTR_INIT_POWER_ON_STATE]		= { 13, 3  },
	[LPDDR4_INTR_MRR_ERROR]				= { 14, 0  },
	[LPDDR4_INTR_MR_READ_DONE]			= { 14, 2  },
	[LPDDR4_INTR_MR_WRITE_DONE]			= { 14, 3  },
	[LPDDR4_INTR_PARITY_ERROR]			= { 15, 2  },
	[LPDDR4_INTR_LOR_BITS]				= { 16, 0  },
};

void ti_lpddr4_enable_pi_initiator(const ti_lpddr4_privatedata *pd)
{
	uint32_t regval = 0U;

	lpddr4_ctlregs *ctlregbase = pd->ctlbase;

	regval = CPS_FLD_SET(TI_LPDDR4__PI_NORMAL_LVL_SEQ__FLD,
			     ctlregbase->TI_LPDDR4__PI_NORMAL_LVL_SEQ__REG);
	ctlregbase->TI_LPDDR4__PI_NORMAL_LVL_SEQ__REG = regval;
	regval = CPS_FLD_SET(TI_LPDDR4__PI_INIT_LVL_EN__FLD,
			     ctlregbase->TI_LPDDR4__PI_INIT_LVL_EN__REG);
	ctlregbase->TI_LPDDR4__PI_INIT_LVL_EN__REG = regval;
}

uint32_t ti_lpddr4_checkctlinterrupt(const ti_lpddr4_privatedata *pd,
				     ti_lpddr4_intr_ctlinterrupt intr,
				     bool *irqstatus)
{
	uint32_t result;
	uint32_t ctlmasterirqstatus = 0U;
	uint32_t ctlgrpirqstatus = 0U;
	uint32_t ctlmasterintflag = 0U;

	result = ti_lpddr4_intr_ctlint_sf(pd, intr, irqstatus);
	if (result != 0U) {
		return result;
	}

	lpddr4_ctlregs *ctlregbase = pd->ctlbase;

	ctlmasterirqstatus =
		(ctlregbase->TI_LPDDR4__INT_STATUS_MASTER__REG &
		 (~LPDDR4_INT_STATUS_MASTER_MASK));

	switch (intr) {
	case LPDDR4_INTR_TIMEOUT_ZQ_CAL_INIT:
	case LPDDR4_INTR_TIMEOUT_ZQ_CALLATCH:
	case LPDDR4_INTR_TIMEOUT_ZQ_CALSTART:
	case LPDDR4_INTR_TIMEOUT_MRR_TEMP:
	case LPDDR4_INTR_TIMEOUT_DQS_OSC_REQ:
	case LPDDR4_INTR_TIMEOUT_DFI_UPDATE:
	case LPDDR4_INTR_TIMEOUT_LP_WAKEUP:
	case LPDDR4_INTR_TIMEOUT_AUTO_REFRESH_MAX:
		ctlgrpirqstatus = ctlregbase->TI_LPDDR4__INT_STATUS_TIMEOUT__REG;
		break;
	case LPDDR4_INTR_LP_DONE:
	case LPDDR4_INTR_LP_TIMEOUT:
		ctlgrpirqstatus = CPS_FLD_READ(TI_LPDDR4__INT_STATUS_LOWPOWER__FLD,
				ctlregbase->TI_LPDDR4__INT_STATUS_LOWPOWER__REG);
		break;
	case LPDDR4_INTR_TRAINING_ZQ_STATUS:
	case LPDDR4_INTR_TRAINING_DQS_OSC_DONE:
	case LPDDR4_INTR_TRAINING_DQS_OSC_UPDATE_DONE:
	case LPDDR4_INTR_TRAINING_DQS_OSC_OVERFLOW:
	case LPDDR4_INTR_TRAINING_DQS_OSC_VAR_OUT:
		ctlgrpirqstatus = ctlregbase->TI_LPDDR4__INT_STATUS_TRAINING__REG;
		break;
	case LPDDR4_INTR_USERIF_OUTSIDE_MEM_ACCESS:
	case LPDDR4_INTR_USERIF_MULTI_OUTSIDE_MEM_ACCESS:
	case LPDDR4_INTR_USERIF_PORT_CMD_ERROR:
	case LPDDR4_INTR_USERIF_WRAP:
	case LPDDR4_INTR_USERIF_INVAL_SETTING:
		ctlgrpirqstatus = ctlregbase->TI_LPDDR4__INT_STATUS_USERIF__REG;
		break;
	case LPDDR4_INTR_MISC_MRR_TRAFFIC:
	case LPDDR4_INTR_MISC_SW_REQ_MODE:
	case LPDDR4_INTR_MISC_CHANGE_TEMP_REFRESH:
	case LPDDR4_INTR_MISC_TEMP_ALERT:
	case LPDDR4_INTR_MISC_REFRESH_STATUS:
		ctlgrpirqstatus = CPS_FLD_READ(TI_LPDDR4__INT_STATUS_MISC__FLD,
				ctlregbase->TI_LPDDR4__INT_STATUS_MISC__REG);
		break;
	case LPDDR4_INTR_BIST_DONE:
		ctlgrpirqstatus = CPS_FLD_READ(TI_LPDDR4__INT_STATUS_BIST__FLD,
				ctlregbase->TI_LPDDR4__INT_STATUS_BIST__REG);
		break;
	case LPDDR4_INTR_DFI_UPDATE_ERROR:
	case LPDDR4_INTR_DFI_PHY_ERROR:
	case LPDDR4_INTR_DFI_BUS_ERROR:
	case LPDDR4_INTR_DFI_STATE_CHANGE:
	case LPDDR4_INTR_DFI_DLL_SYNC_DONE:
	case LPDDR4_INTR_DFI_TIMEOUT:
		ctlgrpirqstatus = CPS_FLD_READ(TI_LPDDR4__INT_STATUS_DFI__FLD,
				ctlregbase->TI_LPDDR4__INT_STATUS_DFI__REG);
		break;
	case LPDDR4_INTR_FREQ_DFS_REQ_HW_IGNORE:
	case LPDDR4_INTR_FREQ_DFS_HW_TERMINATE:
	case LPDDR4_INTR_FREQ_DFS_HW_DONE:
	case LPDDR4_INTR_FREQ_DFS_REQ_SW_IGNORE:
	case LPDDR4_INTR_FREQ_DFS_SW_TERMINATE:
	case LPDDR4_INTR_FREQ_DFS_SW_DONE:
		ctlgrpirqstatus = CPS_FLD_READ(TI_LPDDR4__INT_STATUS_FREQ__FLD,
				ctlregbase->TI_LPDDR4__INT_STATUS_FREQ__REG);
		break;
	case LPDDR4_INTR_INIT_MEM_RESET_DONE:
	case LPDDR4_INTR_MC_INIT_DONE:
	case LPDDR4_INTR_INIT_POWER_ON_STATE:
		ctlgrpirqstatus = CPS_FLD_READ(TI_LPDDR4__INT_STATUS_INIT__FLD,
				ctlregbase->TI_LPDDR4__INT_STATUS_INIT__REG);
		break;
	case LPDDR4_INTR_MRR_ERROR:
	case LPDDR4_INTR_MR_READ_DONE:
	case LPDDR4_INTR_MR_WRITE_DONE:
		ctlgrpirqstatus = ctlregbase->TI_LPDDR4__INT_STATUS_MODE__REG;
		break;
	case LPDDR4_INTR_PARITY_ERROR:
		ctlgrpirqstatus = CPS_FLD_READ(TI_LPDDR4__INT_STATUS_PARITY__FLD,
				ctlregbase->TI_LPDDR4__INT_STATUS_PARITY__REG);
		break;
	default:
		ctlmasterintflag = 1U;
		break;
	}

	if ((ctlintmap[intr].int_shift < TI_WORD_SHIFT) &&
	    (ctlintmap[intr].grp_shift < TI_WORD_SHIFT)) {
		if ((((ctlmasterirqstatus >> ctlintmap[intr].grp_shift) &
		      TI_LPDDR4_BIT_MASK) > 0U) &&
		    (((ctlgrpirqstatus >> ctlintmap[intr].int_shift) &
		      TI_LPDDR4_BIT_MASK) > 0U) &&
		    (ctlmasterintflag == 0U))
			*irqstatus = true;
		else if ((((ctlmasterirqstatus >> ctlintmap[intr].grp_shift) &
			   TI_LPDDR4_BIT_MASK) > 0U) &&
			 (ctlmasterintflag == 1U))
			*irqstatus = true;
		else
			*irqstatus = false;
	}

	return result;
}

uint32_t ti_lpddr4_ackctlinterrupt(const ti_lpddr4_privatedata *pd,
				   ti_lpddr4_intr_ctlinterrupt intr)
{
	uint32_t result;
	uint32_t regval = 0U;
	lpddr4_ctlregs *ctlregbase;

	result = ti_lpddr4_intr_ack_ctlint_sf(pd, intr);
	if (result != 0U) {
		return result;
	}
	if (ctlintmap[intr].int_shift >= TI_WORD_SHIFT) {
		return result;
	}

	ctlregbase = pd->ctlbase;

	switch (intr) {
	case LPDDR4_INTR_TIMEOUT_ZQ_CAL_INIT:
	case LPDDR4_INTR_TIMEOUT_ZQ_CALLATCH:
	case LPDDR4_INTR_TIMEOUT_ZQ_CALSTART:
	case LPDDR4_INTR_TIMEOUT_MRR_TEMP:
	case LPDDR4_INTR_TIMEOUT_DQS_OSC_REQ:
	case LPDDR4_INTR_TIMEOUT_DFI_UPDATE:
	case LPDDR4_INTR_TIMEOUT_LP_WAKEUP:
	case LPDDR4_INTR_TIMEOUT_AUTO_REFRESH_MAX:
		ctlregbase->TI_LPDDR4__INT_ACK_TIMEOUT__REG =
			TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift;
		break;
	case LPDDR4_INTR_LP_DONE:
	case LPDDR4_INTR_LP_TIMEOUT:
		regval = CPS_FLD_WRITE(TI_LPDDR4__INT_ACK_LOWPOWER__FLD,
				       ctlregbase->TI_LPDDR4__INT_ACK_LOWPOWER__REG,
				       (TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift));
		ctlregbase->TI_LPDDR4__INT_ACK_LOWPOWER__REG = regval;
		break;
	case LPDDR4_INTR_TRAINING_ZQ_STATUS:
	case LPDDR4_INTR_TRAINING_DQS_OSC_DONE:
	case LPDDR4_INTR_TRAINING_DQS_OSC_UPDATE_DONE:
	case LPDDR4_INTR_TRAINING_DQS_OSC_OVERFLOW:
	case LPDDR4_INTR_TRAINING_DQS_OSC_VAR_OUT:
		ctlregbase->TI_LPDDR4__INT_ACK_TRAINING__REG =
			TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift;
		break;
	case LPDDR4_INTR_USERIF_OUTSIDE_MEM_ACCESS:
	case LPDDR4_INTR_USERIF_MULTI_OUTSIDE_MEM_ACCESS:
	case LPDDR4_INTR_USERIF_PORT_CMD_ERROR:
	case LPDDR4_INTR_USERIF_WRAP:
	case LPDDR4_INTR_USERIF_INVAL_SETTING:
		ctlregbase->TI_LPDDR4__INT_ACK_USERIF__REG =
			TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift;
		break;
	case LPDDR4_INTR_MISC_MRR_TRAFFIC:
	case LPDDR4_INTR_MISC_SW_REQ_MODE:
	case LPDDR4_INTR_MISC_CHANGE_TEMP_REFRESH:
	case LPDDR4_INTR_MISC_TEMP_ALERT:
	case LPDDR4_INTR_MISC_REFRESH_STATUS:
		ctlregbase->TI_LPDDR4__INT_ACK_MISC__REG =
			TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift;
		break;
	case LPDDR4_INTR_BIST_DONE:
		regval = CPS_FLD_WRITE(TI_LPDDR4__INT_ACK_BIST__FLD,
				       ctlregbase->TI_LPDDR4__INT_ACK_BIST__REG,
				       TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift);
		ctlregbase->TI_LPDDR4__INT_ACK_BIST__REG = regval;
		break;
	case LPDDR4_INTR_DFI_UPDATE_ERROR:
	case LPDDR4_INTR_DFI_PHY_ERROR:
	case LPDDR4_INTR_DFI_BUS_ERROR:
	case LPDDR4_INTR_DFI_STATE_CHANGE:
	case LPDDR4_INTR_DFI_DLL_SYNC_DONE:
	case LPDDR4_INTR_DFI_TIMEOUT:
		ctlregbase->TI_LPDDR4__INT_ACK_DFI__REG =
			TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift;
		break;
	case LPDDR4_INTR_FREQ_DFS_REQ_HW_IGNORE:
	case LPDDR4_INTR_FREQ_DFS_HW_TERMINATE:
	case LPDDR4_INTR_FREQ_DFS_HW_DONE:
	case LPDDR4_INTR_FREQ_DFS_REQ_SW_IGNORE:
	case LPDDR4_INTR_FREQ_DFS_SW_TERMINATE:
	case LPDDR4_INTR_FREQ_DFS_SW_DONE:
		regval = CPS_FLD_WRITE(TI_LPDDR4__INT_ACK_FREQ__FLD,
				       ctlregbase->TI_LPDDR4__INT_ACK_FREQ__REG,
				       (TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift));
		ctlregbase->TI_LPDDR4__INT_ACK_FREQ__REG = regval;
		break;
	case LPDDR4_INTR_INIT_MEM_RESET_DONE:
	case LPDDR4_INTR_MC_INIT_DONE:
	case LPDDR4_INTR_INIT_POWER_ON_STATE:
		regval = CPS_FLD_WRITE(TI_LPDDR4__INT_ACK_INIT__FLD,
				       ctlregbase->TI_LPDDR4__INT_ACK_INIT__REG,
				       (TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift));
		ctlregbase->TI_LPDDR4__INT_ACK_INIT__REG = regval;
		break;
	case LPDDR4_INTR_MRR_ERROR:
	case LPDDR4_INTR_MR_READ_DONE:
	case LPDDR4_INTR_MR_WRITE_DONE:
		ctlregbase->TI_LPDDR4__INT_ACK_MODE__REG =
			TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift;
		break;
	case LPDDR4_INTR_PARITY_ERROR:
		regval = CPS_FLD_WRITE(TI_LPDDR4__INT_ACK_PARITY__FLD,
				       ctlregbase->TI_LPDDR4__INT_ACK_PARITY__REG,
				       TI_LPDDR4_BIT_MASK << ctlintmap[intr].int_shift);
		ctlregbase->TI_LPDDR4__INT_ACK_PARITY__REG = regval;
		break;
	default:
		break;
	}

	return result;
}
