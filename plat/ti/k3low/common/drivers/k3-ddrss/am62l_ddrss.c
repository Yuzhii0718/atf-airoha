// SPDX-License-Identifier: BSD-3-Clause
/*
 * Wrapper for Cadence DDR Driver
 *
 * Copyright (C) 2025-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#include <errno.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#include <am62l_psc_minimal.h>
#include <am62lx_ddr_config.h>
#include "cps_drv_lpddr4.h"
#include <k3_console.h>
#include "lpddr4_ctl_regs.h"
#include "lpddr4_if.h"
#include "lpddr4_obj_if.h"
#include "lpddr4_structs_if.h"
#include <platform_def.h>

#define HERTZ_PER_MEGAHERTZ 1000000UL
#define HERTZ_PER_GIGAHERTZ 1000000000UL
#define MAIN_PLL0_VCO_FREQ (2UL * HERTZ_PER_GIGAHERTZ)
#define MAIN_PLL0_HSDIV2_MAX_FREQ (400UL * HERTZ_PER_MEGAHERTZ)

#define DDRSS_CTL_CFG 0x0f308000
#define DDRSS_CTRL_MMR 0x43040000
#define DDRSS_SS_CFG 0x0f300000
#define DDR4_FSP_CLKCHNG_REQ 0x80
#define DDR4_FSP_CLKCHNG_ACK 0x84

#define MAIN_PLL0_CFG 0x04060000
#define MAIN_PLL0_HSDIV2_CTRL 0x88
#define PLL_HSDIV_RESET_MASK GENMASK(31, 31)
#define PLL_HSDIV_DIV_MASK GENMASK(6, 0)
#define PLL_HSDIV_CLKOUT_MASK GENMASK(15, 15)
#define DDR4_CLKCHNG_REQ_MASK GENMASK(7, 7)
#define DDR4_CLKCHNG_REQ_ACTIVE 0x01U
#define DDR4_CLKCHNG_REQ_INACTIVE 0x00U
#define DDR4_CLKCHNG_REQTYPE_MASK GENMASK(1, 0)
#define DDR4_CLKCHNG_REQ_FREQ1 0x01U
#define DDR4_CLKCHNG_REQ_FREQ2 0x02U
#define DDR4_CLKCHNG_REQ_FREQ0 0x00U
#define DDR4_CLKCHNG_ACK_DONE 0x01U
#define DDR4_CLKCHNG_ACK_NONE 0x00U

#define DDRSS_PI_REGISTER_BLOCK__OFFS 0x2000
#define DDRSS_PI_83__SFR_OFFS 0x14C
#define DDRSS_CTL_342__SFR_OFFS 0x558
#define DENALI_CTL_0_DRAM_CLASS_LPDDR4 0xBU

#define DDRSS_V2A_CTL_REG 0x0020

#define DDRSS_V2A_CTL_REG_SDRAM_IDX_CALC(x) ((ddrss_log2_floor(x) - 16) << 5)
#define DDRSS_V2A_CTL_REG_SDRAM_IDX_MASK (~(0x1F << 5))

#define PD_DDR			2
#define LPSC_MAIN_DDR_LOCAL	21
#define LPSC_MAIN_DDR_CFG_ISO_N	22
#define LPSC_MAIN_DDR_DATA_ISO_N	23

#define str(s) #s
#define xstr(s) str(s)
#define TH_OFFSET_FROM_REG(REG, offset) do { \
		char *i, *pstr = xstr(REG); offset = 0; \
		for (i = pstr; *i != '\0' && *i != '['; ++i) \
			; \
		if (*i == '[') { \
			for (++i; *i != '\0' && *i != ']'; ++i) { \
				offset = offset * 10 + (*i - '0'); } \
		} \
} while (0)

struct k3_ddrss_desc {
	void *ddrss_ss_cfg;
	void *ddrss_ctrl_mmr;
	void *ddrss_ctl_cfg;
	uint32_t ddr_freq0;
	uint32_t ddr_freq1;
	uint32_t ddr_freq2;
	uint32_t ddr_fhs_cnt;
	uint32_t dram_class;
	uint32_t instance;
	ti_lpddr4_obj *driverdt;
	ti_lpddr4_config config;
	ti_lpddr4_privatedata pd;
	uint64_t ecc_reserved_space;
	bool ti_ecc_enabled;
};

static struct k3_ddrss_desc ddrss;

static inline uint64_t ddrss_log2_floor(uint64_t n)
{
	uint64_t val;

	for (val = 0; n > 1U; val++, n >>= 1)
		;

	return val;
}

/************************************************
 * Function to set PLL divider for DDR SS clocks
 ************************************************/
static int set_ddr_pll_div(unsigned int div)
{
	volatile uint32_t reg_val;
	uint32_t set_val;

	/* Set reset to high */
	reg_val = mmio_read_32((uintptr_t)(MAIN_PLL0_CFG + MAIN_PLL0_HSDIV2_CTRL));
	set_val = (reg_val & ~PLL_HSDIV_RESET_MASK) | FIELD_PREP(PLL_HSDIV_RESET_MASK, 1);
	mmio_write_32((uintptr_t)(MAIN_PLL0_CFG + MAIN_PLL0_HSDIV2_CTRL), set_val);

	/* Set divider value */
	reg_val = mmio_read_32((uintptr_t)(MAIN_PLL0_CFG + MAIN_PLL0_HSDIV2_CTRL));
	set_val = (reg_val & ~PLL_HSDIV_DIV_MASK) | FIELD_PREP(PLL_HSDIV_DIV_MASK, div);
	mmio_write_32((uintptr_t)(MAIN_PLL0_CFG + MAIN_PLL0_HSDIV2_CTRL), set_val);

	/* Set enable bit */
	reg_val = mmio_read_32((uintptr_t)(MAIN_PLL0_CFG + MAIN_PLL0_HSDIV2_CTRL));
	set_val = (reg_val & ~PLL_HSDIV_CLKOUT_MASK) | FIELD_PREP(PLL_HSDIV_CLKOUT_MASK, 1);
	mmio_write_32((uintptr_t)(MAIN_PLL0_CFG + MAIN_PLL0_HSDIV2_CTRL), set_val);

	/* clear reset */
	reg_val = mmio_read_32((uintptr_t)(MAIN_PLL0_CFG + MAIN_PLL0_HSDIV2_CTRL));
	set_val = (reg_val & ~PLL_HSDIV_RESET_MASK) | FIELD_PREP(PLL_HSDIV_RESET_MASK, 0);
	mmio_write_32((uintptr_t)(MAIN_PLL0_CFG + MAIN_PLL0_HSDIV2_CTRL), set_val);

	return 0;
}

/*******************************
 * Function to set DDRSS PLL
 ******************************/
static int ddrss_set_pll(unsigned long freq)
{
	int ret = 0;
	unsigned int div;

	if (freq == 0UL || freq > MAIN_PLL0_HSDIV2_MAX_FREQ) {
		return -1;
	}

	/* set HSDIV divider. Set less by 1 as HSDIV adds a one. */
	div = MAIN_PLL0_VCO_FREQ/freq;
	div -= 1;
	ret = set_ddr_pll_div(div);

	return ret;
}

/*************************************************************************
 * Function to change DDRSS PLL clock. It is called by the lpddr4 driver
 * during training
 ************************************************************************/
static void k3_lpddr4_freq_update(struct k3_ddrss_desc *ddr)
{
	volatile uint32_t reg_val;
	uint32_t req = 0;
	uint32_t counter = 0;
	uint32_t req_type;

	for (counter = 0; counter < ddr->ddr_fhs_cnt; counter++) {
		/* update the PLL divisor for DDR PLL */
		reg_val = mmio_read_32(DDRSS_CTRL_MMR + DDR4_FSP_CLKCHNG_REQ);
		req = FIELD_GET(DDR4_CLKCHNG_REQ_MASK, reg_val);
		while (req == DDR4_CLKCHNG_REQ_INACTIVE) {
			reg_val = mmio_read_32((uintptr_t)(DDRSS_CTRL_MMR + DDR4_FSP_CLKCHNG_REQ));
			req = FIELD_GET(DDR4_CLKCHNG_REQ_MASK, reg_val);
		}

		reg_val = mmio_read_32((uintptr_t)(DDRSS_CTRL_MMR + DDR4_FSP_CLKCHNG_REQ));
		req_type = FIELD_GET(DDR4_CLKCHNG_REQTYPE_MASK, reg_val);
		if (req_type == DDR4_CLKCHNG_REQ_FREQ1)
			ddrss_set_pll(ddr->ddr_freq1);
		else if (req_type == DDR4_CLKCHNG_REQ_FREQ2)
			ddrss_set_pll(ddr->ddr_freq2);
		else if (req_type == DDR4_CLKCHNG_REQ_FREQ0)
			ddrss_set_pll(ddr->ddr_freq0);
		else
			WARN("invalid DDR freq request type\n");

		mmio_write_32((uintptr_t)(DDRSS_CTRL_MMR + DDR4_FSP_CLKCHNG_ACK),
			      DDR4_CLKCHNG_ACK_DONE);
		reg_val = mmio_read_32((uintptr_t)(DDRSS_CTRL_MMR + DDR4_FSP_CLKCHNG_REQ));
		req = FIELD_GET(DDR4_CLKCHNG_REQ_MASK, reg_val);
		while (req == DDR4_CLKCHNG_REQ_ACTIVE) {
			reg_val = mmio_read_32((uintptr_t)(DDRSS_CTRL_MMR + DDR4_FSP_CLKCHNG_REQ));
			req = FIELD_GET(DDR4_CLKCHNG_REQ_MASK, reg_val);
		}
		mmio_write_32((uintptr_t)(DDRSS_CTRL_MMR + DDR4_FSP_CLKCHNG_ACK),
			      DDR4_CLKCHNG_ACK_NONE);
	}
	INFO("DDR Freq change complete\n");
}

/*************************************************************************
 * Function to change DDRSS PLL clock. It is called by the lpddr4 driver
 * during training
 ************************************************************************/
static void k3_lpddr4_ack_freq_upd_req(const ti_lpddr4_privatedata *pd)
{
	if (ddrss.dram_class == DENALI_CTL_0_DRAM_CLASS_LPDDR4)
		k3_lpddr4_freq_update(&ddrss);
}

static void k3_lpddr4_info_handler(const ti_lpddr4_privatedata *pd,
				   ti_lpddr4_infotype infotype)
{
	if (infotype == LPDDR4_DRV_SOC_PLL_UPDATE)
		k3_lpddr4_ack_freq_upd_req(pd);
}

/*************************************************************************
 * Function to change DDRSS PLL clock. It is called by the lpddr4 driver
 * during training
 ************************************************************************/
int am62l_lpddr4_init(void)
{
	uint16_t configsize = 0U;
	uint32_t status = 0U;
	ti_lpddr4_config *config = &ddrss.config;
	ti_lpddr4_privatedata *pd = &ddrss.pd;
	ti_lpddr4_obj *driverdt;
	uint32_t offset = 0;
	uint32_t regval;
	uint32_t sdram_idx;
	uint32_t v2a_ctl_reg;
	uint64_t ddr_ram_size;
	int ret;

	ddrss.ddr_fhs_cnt = am62lx_ddr_cfg.ddr_fhs_cnt;
	ddrss.ddr_freq0   = am62lx_ddr_cfg.ddr_freq0;
	ddrss.ddr_freq1   = am62lx_ddr_cfg.ddr_freq1;
	ddrss.ddr_freq2   = am62lx_ddr_cfg.ddr_freq2;
	ddr_ram_size      = AM62L_DDR_RAM_SIZE;

	ddrss.dram_class = CPS_FLD_READ(TI_LPDDR4__DRAM_CLASS__FLD,
					am62lx_ddr_cfg.ctl_data[0]);
	NOTICE("BL1: dram_class: %d\n", ddrss.dram_class);

	if (ddrss.dram_class == DENALI_CTL_0_DRAM_CLASS_LPDDR4)
		ret = ddrss_set_pll(ddrss.ddr_freq0);
	else
		ret = ddrss_set_pll(ddrss.ddr_freq1);

	if (ret != 0)
		return ret;

	/* Disable the DDR LPSCs to start in known state */
	ret = set_main_psc_state(PD_DDR, LPSC_MAIN_DDR_DATA_ISO_N,
				 PSC_PD_ON, PSC_MD_SWRESETDISABLE);
	if (ret != 0)
		return ret;

	ret = set_main_psc_state(PD_DDR, LPSC_MAIN_DDR_CFG_ISO_N,
				 PSC_PD_ON, PSC_MD_SWRESETDISABLE);
	if (ret != 0)
		return ret;

	ret = set_main_psc_state(PD_DDR, LPSC_MAIN_DDR_LOCAL,
				 PSC_PD_OFF, PSC_MD_SWRESETDISABLE);
	if (ret != 0)
		return ret;

	/* Enable DDR LPSCs to configure the controllers */
	ret = set_main_psc_state(PD_DDR, LPSC_MAIN_DDR_LOCAL, PSC_PD_ON, PSC_MD_ENABLE);
	if (ret != 0)
		return ret;

	ret = set_main_psc_state(PD_DDR, LPSC_MAIN_DDR_CFG_ISO_N, PSC_PD_ON, PSC_MD_ENABLE);
	if (ret != 0)
		return ret;

	ret = set_main_psc_state(PD_DDR, LPSC_MAIN_DDR_DATA_ISO_N, PSC_PD_ON, PSC_MD_ENABLE);
	if (ret != 0)
		return ret;

	ddrss.ddrss_ctl_cfg = (void *)DDRSS_CTL_CFG;
	ddrss.ddrss_ctrl_mmr = (void *)DDRSS_CTRL_MMR;
	ddrss.ddrss_ss_cfg = (void *)DDRSS_SS_CFG;

	ddrss.driverdt = ti_lpddr4_getinstance();
	driverdt = ddrss.driverdt;

	status = driverdt->probe(config, &configsize);
	if (status != 0U) {
		ERROR("lpddr4: probe failed status=0x%x\n", status);
		return (int)status;
	}
	INFO("lpddr4: probe done\n");

	/* set LPDDR4 size */
	sdram_idx = DDRSS_V2A_CTL_REG_SDRAM_IDX_CALC(ddr_ram_size);
	v2a_ctl_reg = mmio_read_32((uintptr_t)(DDRSS_SS_CFG + DDRSS_V2A_CTL_REG));
	v2a_ctl_reg = (v2a_ctl_reg & DDRSS_V2A_CTL_REG_SDRAM_IDX_MASK) | sdram_idx;
	mmio_write_32((uintptr_t)(DDRSS_SS_CFG + DDRSS_V2A_CTL_REG), v2a_ctl_reg);

	/* Initialize LPDDR4 */
	config->ctlbase = ddrss.ddrss_ctl_cfg;
	config->infohandler = k3_lpddr4_info_handler;
	status = driverdt->init(pd, config);
	if (status != 0U) {
		ERROR("lpddr4: init failed status=0x%x\n", status);
		return (int)status;
	}
	INFO("lpddr4/ddr4: init done\n");

	/* Configure DDR with config and control structures */
	driverdt->writectlconfigex(pd, am62lx_ddr_cfg.ctl_data,
				   TI_LPDDR4_INTR_CTL_REG_COUNT);
	driverdt->writephyindepconfigex(pd, am62lx_ddr_cfg.pi_data,
					TI_LPDDR4_INTR_PHY_INDEP_REG_COUNT);
	driverdt->writephyconfigex(pd, am62lx_ddr_cfg.phy_data,
				   TI_LPDDR4_INTR_PHY_REG_COUNT);

	TH_OFFSET_FROM_REG(TI_LPDDR4__START__REG, offset);
	driverdt->readreg(pd, LPDDR4_CTL_REGS, offset, &regval);
	if (CPS_FLD_READ(TI_LPDDR4__START__FLD, regval) != 0) {
		ERROR("LPDDR4 prestart failed\n");
		return -ENXIO;
	}

	INFO("lpddr4: Start DDR controller\n");
	status = driverdt->start(pd);
	if (status != 0) {
		ERROR("lpddr4: start failed status = 0x%x\n", status);
		return status;
	}

	/* check poststart status */
	driverdt->readreg(pd, LPDDR4_CTL_REGS, offset, &regval);
	INFO("start-status reg: after =0x%x\n", regval);
	if (CPS_FLD_READ(TI_LPDDR4__START__FLD, regval) != 1) {
		ERROR("LPDDR4 poststart failed\n");
		return -ENXIO;
	}

	return 0;
}
