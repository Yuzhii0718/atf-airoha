/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * AM62Lx DDR configuration data structure
 *
 * Copyright (C) 2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef AM62LX_DDR_CONFIG_H
#define AM62LX_DDR_CONFIG_H

#include <stdint.h>

/* Register array sizes fixed by Denali LPDDR4 controller architecture */
#define AM62L_DDR_CTL_REG_COUNT		(423U)
#define AM62L_DDR_PI_REG_COUNT		(345U)
#define AM62L_DDR_PHY_REG_COUNT		(1406U)

/* DDR RAM size: default 2 GB, overridable at build time */
#ifndef AM62L_DDR_RAM_SIZE
#define AM62L_DDR_RAM_SIZE		0x80000000ULL
#endif

struct am62lx_ddr_config {
	uint32_t ddr_freq0;
	uint32_t ddr_freq1;
	uint32_t ddr_freq2;
	uint32_t ddr_fhs_cnt;
	uint32_t ctl_data[AM62L_DDR_CTL_REG_COUNT];
	uint32_t pi_data[AM62L_DDR_PI_REG_COUNT];
	uint32_t phy_data[AM62L_DDR_PHY_REG_COUNT];
};

extern const struct am62lx_ddr_config am62lx_ddr_cfg;

#endif /* AM62LX_DDR_CONFIG_H */
