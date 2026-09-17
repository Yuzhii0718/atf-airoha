/*
 * Copyright (c) 2014-2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FVP_PRIVATE_H
#define FVP_PRIVATE_H

#include <plat/arm/common/plat_arm.h>

/*******************************************************************************
 * Function and variable prototypes
 ******************************************************************************/

void fvp_config_setup(void);

void fvp_interconnect_init(void);
void fvp_interconnect_enable(void);
void fvp_interconnect_disable(void);
void fvp_timer_init(void);
void fvp_pcpu_init(void);
void fvp_gic_driver_pre_init(void);
void fvp_gicv3_make_rdistrif_rw(unsigned int core_pos);

#endif /* FVP_PRIVATE_H */
