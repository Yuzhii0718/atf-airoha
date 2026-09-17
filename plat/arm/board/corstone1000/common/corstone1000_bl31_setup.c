/*
 * Copyright (c) 2023-2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#if defined(SPD_spmd) && (SPMC_AT_EL3 == 0)
/*
 * A dummy implementation of the platform handler for Group0 secure interrupt.
 */
int plat_spmd_handle_group0_interrupt(uint32_t intid)
{
	(void)intid;
	return -1;
}
#endif /*defined(SPD_spmd) && (SPMC_AT_EL3 == 0)*/

void plat_gic_pre_pcpu_init(unsigned int cpu_idx)
{
}
